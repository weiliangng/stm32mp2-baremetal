#include <stdint.h>

#define USART_BASE  0x400E0000u
#define USART_ISR   (*(volatile uint32_t *)(USART_BASE + 0x1Cu))
#define USART_RDR   (*(volatile uint8_t  *)(USART_BASE + 0x24u))
#define USART_TDR   (*(volatile uint8_t  *)(USART_BASE + 0x28u))

#define RXNE_BIT    (1u << 5)   // RX data ready
#define TXE_BIT     (1u << 7)   // TX register empty

/* -------------------- GPIOJ (PJ1) -------------------- */
#define GPIOJ_BASE   0x442D0000u

#define GPIOJ_MODER   (*(volatile uint32_t *)(GPIOJ_BASE + 0x00u))
#define GPIOJ_OTYPER  (*(volatile uint32_t *)(GPIOJ_BASE + 0x04u))
#define GPIOJ_OSPEEDR (*(volatile uint32_t *)(GPIOJ_BASE + 0x08u))
#define GPIOJ_PUPDR   (*(volatile uint32_t *)(GPIOJ_BASE + 0x0Cu))
#define GPIOJ_IDR     (*(volatile uint32_t *)(GPIOJ_BASE + 0x10u))
#define GPIOJ_ODR     (*(volatile uint32_t *)(GPIOJ_BASE + 0x14u))
#define GPIOJ_BSRR    (*(volatile uint32_t *)(GPIOJ_BASE + 0x18u))

#define PJ1          (1u)               /* pin index */
#define PJ1_SET      (1u << PJ1)        /* BSRR set  */
#define PJ1_RST      (1u << (16u + PJ1))/* BSRR reset*/

/* Configure PJ1 as push-pull output, no pull, high speed */
static inline void gpioj_pj1_init_output(void)
{
    /* MODER: 01 = output for pin 1 (bits [3:2]) */
    GPIOJ_MODER  = (GPIOJ_MODER  & ~((uint32_t)0x3u << (PJ1 * 2u)))
                                 |  ((uint32_t)0x1u << (PJ1 * 2u));

    /* OTYPER: 0 = push-pull for bit 1 */
    GPIOJ_OTYPER = (GPIOJ_OTYPER & ~(1u << PJ1));

    /* OSPEEDR: 10 = high speed (optional; change to 00 if you prefer) */
    GPIOJ_OSPEEDR = (GPIOJ_OSPEEDR & ~((uint32_t)0x3u << (PJ1 * 2u)))
                                   |  ((uint32_t)0x2u << (PJ1 * 2u));

    /* PUPDR: 00 = no pull */
    GPIOJ_PUPDR  = (GPIOJ_PUPDR  & ~((uint32_t)0x3u << (PJ1 * 2u)));
}

static inline void trigger_high(void)
{
    /* BSRR low 16 bits set the corresponding pin */
    GPIOJ_BSRR = PJ1_SET;
}

static inline void trigger_low(void)
{
    /* BSRR high 16 bits reset the corresponding pin */
    GPIOJ_BSRR = PJ1_RST;
}

/* crude delay */
static inline void delay(volatile uint32_t n)
{
    while (n--) __asm__ volatile("nop");
}

void main(void) {
    gpioj_pj1_init_output();

    for (;;) {
        // wait for a byte
        while ((USART_ISR & RXNE_BIT) == 0) {
       	
		trigger_high();
	        //delay(400000);
		trigger_low();
		//delay(400000);

	}
        uint8_t c = USART_RDR;          // reading clears RXNE

        // echo it
        while ((USART_ISR & TXE_BIT) == 0) { }
        USART_TDR = c;
    }
}
