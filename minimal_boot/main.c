#include <stdint.h>

#define USART_BASE  0x400E0000u
#define USART_ISR   (*(volatile uint32_t *)(USART_BASE + 0x1Cu))
#define USART_RDR   (*(volatile uint8_t  *)(USART_BASE + 0x24u))
#define USART_TDR   (*(volatile uint8_t  *)(USART_BASE + 0x28u))

#define RXNE_BIT    (1u << 5)   // RX data ready
#define TXE_BIT     (1u << 7)   // TX register empty

void main(void) {
    for (;;) {
        // wait for a byte
        while ((USART_ISR & RXNE_BIT) == 0) { }
        uint8_t c = USART_RDR;          // reading clears RXNE

        // echo it
        while ((USART_ISR & TXE_BIT) == 0) { }
        USART_TDR = c;
    }
}
