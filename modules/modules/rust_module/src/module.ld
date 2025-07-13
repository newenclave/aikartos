ENTRY(module_entry)

MEMORY
{
  RAM (rwx) : ORIGIN = 0x0, LENGTH = 16K
}

SECTIONS
{
  .text 0: {
    *(.text*)
    *(.rodata*)	
  } > RAM

  .data : {
    *(.data*)
  } > RAM

  .bss : {
    *(.bss*)
    *(COMMON)
  } > RAM
}
