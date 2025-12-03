OUTPUT_FORMAT(elf64-littleaarch64)
OUTPUT_ARCH(aarch64)

ENTRY(_Reset)

MEMORY
{
    RAM (rwx) : ORIGIN = 0x88000000, LENGTH = 1M
}



SECTIONS {
  . = ORIGIN(RAM);
  .text : { *(.text .text.*) } > RAM
  .rodata : { *(.rodata .rodata.*) } > RAM

  .data : ALIGN(16) {
    __data_start__ = .; *(.data .data.*) __data_end__ = .;
  } > RAM
  
  .bss (NOLOAD) : ALIGN(64) {
    . = MAX(., ORIGIN(RAM) + 0x0021000);
    PROVIDE(__gap_cookie = 0xC0FFEE);
    ASSERT(. >= ORIGIN(RAM) + 0x0021000, "bss too low");
    __bss_start__ = .;
    *(.bss .bss.* COMMON)
    __bss_end__   = .;
  } > RAM
  
  . = ALIGN(16);
  __image_end = .;

  /* simple, fixed stack near top of our 1 MiB window (leave 16 KiB guard) */
  __stack_top = ORIGIN(RAM) + LENGTH(RAM) - 0x4000;

  . = ALIGN(16);
  _end = .;
  PROVIDE(end = _end);
}
