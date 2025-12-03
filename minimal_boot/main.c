#include <stdint.h>
//#include <stdio.h>
#include <stddef.h>



#define DELAY_CYCLES_CONST(N) __asm__ __volatile__ (".rept " #N "\n\tnop\n\t.endr\n" ::: "memory")
#define COMPILER_BARRIER() __asm__ volatile ("" ::: "memory")

static __attribute__((noinline)) void delay_nops_runtime(uint32_t iters) {
	asm volatile(
			"1:\n\t"
			"subs   %w0, %w0, #1\n\t"  // decrement
			"nop\n\t"                  // the requested NOP per iteration
			"b.ne   1b\n\t"            // loop if not zero
			: "+r"(iters)
			:
			: "cc", "memory");
}


//Select one mode: MODE_COUNTING_LOOP / MODE_WHILE / MODE_WHILE_NO_TIMEOUT / MODE_COMPARISON / MODE_ECHO
#define MODE_COMPARISON

/* Worst case for uint64_t is: 'r' + 20 digits + '\0' = 22 bytes */
#define R_U64_BUFSZ 22u

//#define USART_BASE  0x400E0000u//console
#define USART_BASE  0x40220000u //header pins


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

static inline void usart_putc(uint8_t c) {
	while ((USART_ISR & TXE_BIT) == 0) { }
	USART_TDR = c;
}

static inline uint8_t usart_getc(void) {
	while ((USART_ISR & RXNE_BIT) == 0) { }
	return USART_RDR;                 // read clears RXNE
}

// helper: send n chars
static void uart_send_buf(const char *buf, int n) {
	if (n <= 0) return;
	for (int i = 0; i < n; ++i) usart_putc((uint8_t)buf[i]);
}

/* Convert unsigned 8..64-bit value to "r" + decimal digits.
   Returns length (without NUL) on success, 0 on insufficient dst_cap. */

static inline size_t rfmt_u64(char *dst, size_t dst_cap, uint64_t v) {
	char rev[20];
	size_t n = 0;
	do { rev[n++] = (char)('0' + (v % 10u)); v /= 10u; } while (v);
	if (dst_cap < (1u + n + 1u)) return 0;
	dst[0] = 'r';
	for (size_t i = 0; i < n; ++i) dst[1 + i] = rev[n - 1 - i];
	dst[1 + n] = '\0';
	return 1u + n;
}

//prints integer over uart as ascii characters
static inline void print_u64_dec(uint64_t x) {
	char b[R_U64_BUFSZ];
	size_t n = rfmt_u64(b, sizeof(b), x);
	if (n) uart_send_buf(b, (int)n);
}


// ========================= MODES =========================
#if defined(MODE_COUNTING_LOOP)

static uint32_t run_mode(void) {
	volatile uint32_t cnt = 0;

	trigger_high();

	for (volatile uint32_t i = 0; i < 99; i++) {
		for (volatile uint32_t j = 0; j < 99; j++) {
			cnt++;
		}
	}

	trigger_low();

	// expected result: 2500 -> prints r2500
	return cnt;
}

#elif defined(MODE_WHILE)

static uint32_t run_mode(void) {
	volatile uint32_t  ok    = 5;
	volatile uint32_t tries = 2500;

	trigger_high();

	while ((ok != 2) && (tries > 0)) {
		tries--;
		COMPILER_BARRIER();
	}

	trigger_low();

	if (tries > 0) {
		return tries;
	} else {
		return 9999;
	}
}

#elif defined(MODE_WHILE_NO_TIMEOUT)
__attribute__((noinline))
	static uint32_t run_mode(void) {

		volatile uint32_t ok = 0;

		trigger_high();
		while (ok != 2) {
			// COMPILER_BARRIER();
		}
		trigger_low();

		return 9999;
	}

#elif defined(MODE_COMPARISON)

static uint32_t run_mode(void) {
	
	volatile uint32_t ok = 1;
	//uint32_t hits = 0;         // counts how many times ok==2
	// --- Precompute GPIO address + values, outside the hot path ---
        //register uint32_t *gpio_bsrr asm("x20") = (uint32_t *)0x442D0018u;  // GPIOJ_BSRR address
        //register uint32_t pj1_rst asm("w21") = PJ1_RST;  // 0x20000
        //register uint32_t pj1_set asm("w22") = PJ1_SET;  // 0x00002
	//COMPILER_BARRIER();  // stop the compiler reordering across this point
	
	trigger_high();
	delay_nops_runtime(30);
	trigger_low();
	
	delay_nops_runtime(2500);
	
	trigger_high();
	DELAY_CYCLES_CONST(200);
	
	// One identical measurement block (branchless result accumulation)
#define PROBE_BLOCK()                          \
	__asm__ volatile ("" ::: "memory");        \
	if (ok == 2) return 0; /* return */        \

	// Unroll helpers
#define REP10(X) X; X; X; X; X; X; X; X; X; X
#define REP100(X) REP10(REP10(X))
#define REP1000(X) REP10(REP100(X))

	//trigger_low();
	//GPIOJ_BSRR = PJ1_RST;
	// Falling edge: *just* a store, no mov/movk
	//__asm__ volatile("str w21, [x20]" ::: "memory");
	
	// Do it 10 times, no loop overhead
	REP10(PROBE_BLOCK());    
	//PROBE_BLOCK();
	//trigger_high();
	//GPIOJ_BSRR = PJ1_SET;
	// Rising edge: again just a store
        //__asm__ volatile("str w22, [x20]" ::: "memory");

	DELAY_CYCLES_CONST(200);

	trigger_low();

	return 9999;
}

#elif defined(MODE_ECHO)

//echo loop do not select
static void run_mode(void) {
	for (;;) {
		while ((USART_ISR & RXNE_BIT) == 0) { }
		uint8_t c = USART_RDR;
		usart_putc(c);
	}
}


#elif defined(MODE_STR)

/* Store & verify: writes 100 words to a static RAM array,
   then scans for corruptions. */
static volatile uint32_t test_area[100] __attribute__((aligned(64)));

#define NOP_PRE     100
#define STORE_CNT   100
#define NOP_POST    100

static uint32_t run_mode(void) {
	volatile uint32_t *base = &test_area[0];
	const uint32_t val = 0xDEADBEEFu;

	COMPILER_BARRIER();
	trigger_high();
	__asm__ volatile(
			// setup (before the strict region)
			"mov x0, %0\n\t"           // x0 = base
			"mov w2, %w1\n\t"          // w2 = val

			// --- NOP prelude ---
			".rept %c2\n\t"
			"nop\n\t"
			".endr\n\t"

			// --- STR-only burst ---
			".rept %c3\n\t"
			"str w2, [x0], #4\n\t"
			".endr\n\t"

			// --- NOP postlude ---
			".rept %c4\n\t"
			"nop\n\t"
			".endr\n\t"
			:
			: "r"(base),       // %0
		"r"(val),        // %1
		"i"(NOP_PRE),    // %2  (immediate)
		"i"(STORE_CNT),  // %3  (immediate)
		"i"(NOP_POST)    // %4  (immediate)
			: "x0", "w2", "memory");
	trigger_low();
	test_area[50] = 1;
	// verify...
	uint32_t bad = 0;
	for (uint32_t i = 0; i < STORE_CNT; ++i) {
		uint32_t actual = test_area[i];
		if (actual != val) {
			uintptr_t addr = (uintptr_t)&test_area[i];
			print_u64_dec((uint64_t)addr);
			usart_putc(':');
			print_u64_dec((uint64_t)actual);
			++bad;
		}
	}
	return bad;
}

#else
# error "Select one mode: MODE_COUNTING_LOOP / MODE_WHILE / MODE_WHILE_NO_TIMEOUT / MODE_COMPARISON / MODE_ECHO"
#endif


void main(void) {
	gpioj_pj1_init_output();
	usart_putc('s');//ready signal

	while(1){
		while (usart_getc() != 'g') {}
		uint32_t res = run_mode();
		trigger_low();
		char buf[R_U64_BUFSZ];
		size_t n = rfmt_u64(buf, sizeof(buf), (uint64_t)res);
		if (n) uart_send_buf(buf, (int)n);
	}

}
