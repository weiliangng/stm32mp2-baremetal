OUTPUT_FORMAT(elf64-littleaarch64)
OUTPUT_ARCH(aarch64)

ENTRY(_Reset)

MEMORY
{
    ROM (rx) : ORIGIN = 0x88000000, LENGTH = 1M
}



SECTIONS {
  . = ORIGIN(ROM);
  .text : { *(.text .text.*) } > ROM
  .rodata : { *(.rodata .rodata.*) } > ROM

  .data : ALIGN(16) {
    __data_start__ = .; *(.data .data.*) __data_end__ = .;
  } > ROM

  .bss (NOLOAD) : ALIGN(16) {
    __bss_start__ = .; *(.bss .bss.* COMMON) __bss_end__ = .;
  } > ROM

  . = ALIGN(16);
  __image_end = .;

  /* simple, fixed stack near top of our 1 MiB window (leave 16 KiB guard) */
  __stack_top = ORIGIN(ROM) + LENGTH(ROM) - 0x4000;
}
