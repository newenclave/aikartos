# AikaRTOS Modules and Bundle Format

AikaRTOS supports dynamically loadable modules and a simple bundle format to group multiple modules together. This system is great for embedded use cases where you want flexibility and runtime extension without full firmware reflash.

---

## Modules Overview

A **module** is a self-contained binary file containing code, data, and metadata. It is generated using `create_bin.py` from a compiled ELF file. Modules are loaded into memory, relocated using internal tables, and receive a pointer to the AikaRTOS API.

### Module File Layout

```
[ Header (64B) ][ Description? ][ .text/.rodata/.data ][ Relocations ][ Symbols ]
```

* All sections are aligned to 8 bytes
* `.bss` is not stored in the binary - it is declared in the header (size + offset)

### Module Header

```cpp
struct section {
    uint32_t offset;
    uint32_t size;
};

struct header {
    uint32_t signature;      // 'AIKM'
    uint32_t version;        // format version
    section binary;          // code/data/rodata block
    section relocs;          // relocation table
    section symbols;         // symbol table
    section bss;             // .bss section (in-memory only)
    uint32_t crc;            // CRC32 over entire module
    uint32_t total_size;     // total size of module binary
    uint32_t entry_offset;   // offset to entry point function
    uint32_t reserved[3];
};
```

### Relocation Entry

```cpp
struct relocation {
    uint32_t offset;
    uint32_t type;          // e.g. R_ARM_ABS32
    uint32_t section_idx;
    uint32_t symbol_idx;
};
```

### Symbol Entry

```cpp
struct symbol {
    uint32_t value;         // symbol address
    uint32_t section_idx;
    uint32_t type;          // STT_FUNC, STT_OBJECT...
    uint32_t reserved;
};
```

---

## Bundle Format

A **bundle** is a container for multiple modules. This is useful when you want to load several modules at once - like a task + device driver + config blob.

### File Layout

```
[ Bundle Header ][ Offsets ][ Module 0 ][ padding ][ Module 1 ][ padding ] ...
```

* Each module is aligned to 8 bytes
* All offsets are relative to the beginning of the file

### Bundle Header

```cpp
struct bundle_header {
    uint32_t signature;      // 'AIKB'
    uint32_t modules_count;  // number of modules in bundle
    uint32_t reserved[6];    // must be 0
};
```

Followed by:

```cpp
uint32_t module_offsets[modules_count];
```

Each value is the offset of a corresponding `AIKM` module from the start of the file.

---

## Runtime Loading Example

```cpp
uintptr_t base = (uintptr_t)&_modules_begin;
if (is_module(base)) {
    module m(base);
    m.load(exec_mem);
    add_task(m.get_entry_point(), &api);
} else if (is_bundle(base)) {
    bundle b(base);
    auto m0 = b.get_module(0);
    m0->load(mem0);
    add_task(m0->get_entry_point(), &api);
}
```

---

## Toolchain

* [**create\_bin.py**](build/scripts/create_bin.py): generates `.bin` module from `.elf`
* [**create\_flash\_bundle.py**](build/scripts/create_flash_bundle.py): joins multiple `.bin` files into a bundle

---

## Summary

* Modules are loadable binary units with relocations and symbols
* Bundle = header + offsets + modules
* Modules receive `aikartos_api` at entry point

