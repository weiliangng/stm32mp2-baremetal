#include <stdint.h>
#define USART_BASE  0x400E0000u
#define USART_ISR   (*(volatile uint32_t *)(USART_BASE + 0x1Cu))
#define USART_TDR   (*(volatile uint8_t  *)(USART_BASE + 0x28u))
#define TXE_BIT     (1u << 7)

void main(void) {
    while ((USART_ISR & TXE_BIT) == 0) { }
    USART_TDR = (uint8_t)'A';
    for (;;) __asm__ volatile("wfe");
}
