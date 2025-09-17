    .cpu cortex-a35
    .text
    .global _start
    .global _Reset

/* USART2 base 0x400E0000 => TDR at +0x28 from your dump */
.equ USART_TDR,  0x400E0028

_Reset:
_start:
    /* x0 = TDR address */
    ldr     x0, =USART_TDR   /* may create a pool entry */

    /* write single character 'g' (0x67) as a BYTE */
    mov     w1, #0x67
    strb    w1, [x0]
    
    b       hang          // <-- skip over the literal pool
    
    /* Force any pending literal pool entries to be emitted HERE, before our loop label, so the loop target remains code. */
    .ltorg
    .align 4

hang:
    wfe
    b       hang

