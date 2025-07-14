# Loadable Modules in RTOS: A Practical Guide Using AikaRTOS

This document provides a comprehensive overview of how loadable modules can be implemented in a real-time operating system (RTOS), using **AikaRTOS** as a working example.

Overview:
* Write and compile standalone modules
* Link them correctly for runtime loading
* Package them with relocation info
* Flash them to a target device
* Load and execute them inside an RTOS environment

---

## Table of Contents

1. [Writing a Simple Module](#1-writing-a-simple-module)
2. [Building an ELF with Proper Flags](#2-building-an-elf-with-proper-flags)

   * Compiler & linker options
   * Linker script for modules
3. [Extracting a raw binary](#3-extracting-a-raw-binary)
4. [Parsing the ELF File](#4-parsing-the-elf-file)

   * Getting sections, symbol table, relocations
5. [Building a Custom Binary Format](#5-building-a-custom-binary-format)

   * Header format
   * Layout
   * CRC
6. [Memory Mapping Modules in the RTOS](#6-memory-mapping-modules-in-the-rtos)

   * Linker script region
   * Flash layout
7. [Flashing to the Device](#7-flashing-to-the-device)

   * Using `st-flash`
8. [Loading and Relocating at Runtime](#8-loading-and-relocating-at-runtime)
9. [Additional Notes](#9-additional-notes)

   * Rust support
   * Using CMake + custom commands
   * Limitations & TODOs

---

## 1. Writing a Simple Module

A module is essentially a self-contained binary, built by any suitable compiler (C, C++, Rust, etc.) with the correct flags and layout. It doesn’t require an OS to run but is expected to integrate with the RTOS at runtime. The document describes the process of building such a binary step by step.

### Example (C++)

```cpp
extern "C" int module_entry(void *param) {
    return 0;
}
```

This is all that is required. The function above will act as the `entry_point` for the module.

## 2. Building an ELF with Proper Flags

To produce a valid relocatable module, compile the code with the following properties:

* The output must be an ELF file (`.elf`) with full relocation and symbol information.
* The module must not rely on any external runtime or standard startup files.
* A custom linker script must be used to control layout and set the entry point.

### Compiler example (GCC):

```sh
arm-none-eabi-g++ -ffreestanding -nostdlib -fno-exceptions -fno-rtti \
    -Wall -Wextra \
    -mcpu=cortex-m4 -mthumb \
    -c module.cpp -o module.o
```

* `-mcpu=cortex-m4` and `-mthumb` select the correct instruction set for the target (adjust for specific hardware).
* `-ffreestanding`, `-nostdlib` avoid linking to any host system libraries.
* `-fno-exceptions`, `-fno-rtti` reduce binary size (optional, but useful).

### Linker example:

```sh
arm-none-eabi-ld -T module.ld -nostdlib --emit-relocs -o module.elf module.o
```

* `--emit-relocs` ensures the linker includes relocation entries in the final ELF file.
* `-T module.ld` applies the custom linker script with defined layout.
* `-nostdlib` prevents linking against standard libraries.

### Inspecting the ELF structure

To view ELF sections and confirm that relocation data is present, use the following command:

```sh
arm-none-eabi-objdump -h module.elf
```

This displays the section headers, including `.text`, `.data`, `.bss`, `.rodata`, and any `.rel.*` relocation sections.

Example output:

```
Idx Name          Size      VMA       LMA       File off  Algn
  0 .text         00000040  00000000  00000000  00010000  2**2
  1 .rodata       0000001c  00000040  00000040  00010040  2**2
  2 .data         0000000c  0000005c  0000005c  0001005c  2**2
  3 .bss          00000008  00000068  00000068  00000000  2**2
  4 .rel.text     00000010  00000000  00000000  00010070  2**2
```

Presence of `.rel.*` sections confirms that relocation information has been preserved in the ELF.

If the module does not reference any external symbols or perform operations that require relocation (such as calling functions outside the module), the `.rel.*` sections may be absent. This is normal for simple examples that contain only local logic.

### Example with relocations

To trigger relocation table generation, the module must reference an internal symbol. Example:

```cpp
int call(const char *val) {
    if(*val == 0) {
        return 0;
    }
    return 1;
}

extern "C" int module_entry(void *param) {
    const char *const_string = "This is a string for the relocation";
    return call(const_string);
}
```

This forces the linker to generate a relocation for `const_string` and the call to `call()`.

Inspect the ELF using:

```sh
arm-none-eabi-objdump -x module.elf
```

Example relocation output:

```
RELOCATION RECORDS FOR [.text]:
OFFSET   TYPE              VALUE
0000002e R_ARM_THM_CALL    _Z4callPKc
0000003c R_ARM_ABS32       .text
```

## 3. Extracting a raw binary

To convert the ELF to a flat binary format, use the following command:

```sh
arm-none-eabi-objcopy -O binary \
  -j .text -j .rodata -j .data -j .bss \
  module.elf module.bin
```

This extracts only the relevant sections needed for execution. The `.bss` section is included for size alignment purposes; its content is assumed to be zero-initialized at runtime.

After compilation and binary extraction, the result is a small binary file. For example:

* ELF size: \~5 KB
* BIN size: \~62 bytes


## 4. Parsing the ELF File

Parsing the ELF file is required to extract metadata not available in the raw binary. This includes:

* Entry point address
* Section layout
* Relocation records
* Symbol table
* `.bss` size and location

This step is implementation-specific. In AikaRTOS, a Python script ([`create_bin.py`](build/scripts/create_bin.py)) performs this task using [pyelftools](https://github.com/eliben/pyelftools). Alternatively, parsing can be done manually using the [ELF specification](https://refspecs.linuxfoundation.org/elf/elf.pdf).

The output of the parsing stage is used to build a module package that contains:

* Binary data (.text + .rodata + .data)
* Relocation table
* Symbol table
* Header and metadata

## 5. Building a Custom Binary Format

After parsing the ELF, the next step is to build a custom binary format that is easy to load and interpret inside the RTOS.

The format is implementation-defined. It must contain enough metadata to reconstruct the module in memory and apply all required relocations.

In **AikaRTOS**, each module is packaged with:

* A fixed-size header (with signature, entry point offset, section sizes, relocation table offsets, etc.)
* Optional description string
* Raw code/data blob
* Relocation table
* Symbol table

This format allows the RTOS to:

* Allocate memory
* Copy the module's content into RAM
* Apply relocations using the relocation table
* Call the entry point

A CRC32 checksum and total image size are included to validate integrity.

AikaRTOS provides a Python script (`create_bin.py`) that assembles the final binary. It parses the ELF, extracts the sections, generates relocation and symbol data, and appends them to the binary with proper alignment.

The result is a single binary blob, suitable for flashing or loading at runtime.

## 6. Memory Mapping Modules in the RTOS

In order to store and access modules from flash memory, a separate memory region can be defined in the RTOS linker script.

### Linker layout example (RTOS linker script)

```ld
/* Memories definition */
MEMORY
{
  RAM    (xrw)    : ORIGIN = 0x20000000, LENGTH = 128K
  FLASH  (rx)     : ORIGIN = 0x08000000, LENGTH = 384K
  MODULE (rx)     : ORIGIN = 0x08060000, LENGTH = 128K
}

/* Region marker */
  .modules_begin 0x08060000 :
  {
    KEEP(*(.dummy_for_modules_begin))
  } > MODULE

/* Provide module boundaries */
PROVIDE(_modules_begin = ORIGIN(MODULE));
PROVIDE(_modules_end   = ORIGIN(MODULE) + LENGTH(MODULE));
```

This reserves a block of flash starting at `0x08060000` where modules can be written and later loaded from.

> You can also put the binary on an SD card or wherever you want. The goal is to make it accessible to the RTOS as a block of bytes.  

## 7. Flashing to the Device

### Flashing a compiled module

Use the `st-flash` tool to write the binary to the designated module region:

```sh
st-flash write module.bin 0x08060000
```

This writes the compiled and packaged module to the correct memory location in the device flash. The RTOS will later scan or directly access this region during module loading.

The `module.bin` file may be a raw binary extracted via `objcopy`, or it may be a structured binary produced by a custom script (e.g. `create_bin.py` in AikaRTOS) that includes relocation and symbol tables inside a well-defined format.

## 8. Loading and Relocating at Runtime


Once the module binary is located in flash, it should be loaded and relocated before use.

The typical steps:

1. Allocate a memory region large enough for `.text`, `.data`, `.rodata`, and `.bss`.
2. Copy the contents from flash to RAM.
3. Zero-initialize `.bss`.
4. Iterate over the relocation table and apply each relocation.
5. Call the `entry_point` function.

### Pseudocode Example (C++)

```cpp
uint8_t* dst = allocate_region(total_module_size);
memcpy(dst, module_blob, size_without_bss);
memset(dst + bss_offset, 0, bss_size);

for (auto& rel : relocation_table) {
    apply_relocation(dst, rel);
}

using entry_fn_t = int(*)(void*);
entry_fn_t entry = reinterpret_cast<entry_fn_t>(dst + entry_offset);
entry(api_struct);
```
> It's also possible to use the binary directly from the flash, but remember about the relocations.

### ARM Thumb-2 Relocation Info

ARM uses specific relocation types for Thumb-2 code (e.g., `R_ARM_THM_CALL`, `R_ARM_ABS32`). These define how instruction operands or pointers must be adjusted based on runtime addresses.

Useful references:

* [ARM ELF Relocation Types (sourceware)](https://sourceware.org/binutils/docs-2.34/ld/ARM.html#ARM)
* [ELF for ARM Architecture (ARM IHI 0044)](https://developer.arm.com/documentation/ihi0044/latest)

Correct handling of relocations is essential to ensure valid code execution after loading.

---

## 9. Additional Notes

### Rust Example Module

A simple module can also be written in Rust. It must define an external `module_entry` function and avoid dependencies on the Rust runtime or `std`.

#### Example (Rust)

```rust
#[no_mangle]
pub extern "C" fn module_entry(_param: *mut core::ffi::c_void) -> i32 {
    0
}
```

#### Compiling and Linking (with custom linker)

```sh
rustc --target thumbv7em-none-eabi \
      -C opt-level=z \
      -C linker=arm-none-eabi-ld \
      -C panic=abort \
      -Z build-std=core \
      --emit obj -o module.o module.rs

arm-none-eabi-ld -T module.ld --emit-relocs -o module.elf module.o
```

* `--emit-relocs` ensures relocation table generation.
* `-Z build-std=core` allows using `core` instead of `std`.
* `module.ld` should match the layout described in section 2.

After this, the same `objcopy`, parsing, and packaging steps can be applied.
