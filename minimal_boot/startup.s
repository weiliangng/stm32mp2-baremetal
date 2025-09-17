    .cpu cortex-a35
    .text
    .global _start
    .global _Reset
    .extern main
    .extern __bss_start__
    .extern __bss_end__
    .extern __stack_top



_Reset:
_start:
    // (Optional) own stack; U-Boot usually gives you one
    ldr     x0, =__stack_top
    mov     sp, x0

    // Zero .bss
    ldr     x1, =__bss_start__
    ldr     x2, =__bss_end__
1:  cmp     x1, x2
    b.hs    2f
    str     xzr, [x1], #8
    b       1b
2:
    // Jump to C
    b       main

    // Place literal pools before tight loops (avoids jumping into data)
    .ltorg
    .align 4
