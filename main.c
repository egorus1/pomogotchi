#include <stdint.h>
#include <stdbool.h>
#define BIT(x) (1UL << (x))
#define PIN(bank, num) ((((bank) - 'A') << 8) | (num))
#define PINNO(pin) (pin & 255)
#define PINBANK(pin) (pin >> 8)
#define MAX_DELAY 0xffffff


struct systick {
    volatile uint32_t CSR, RVR, CVR, CALIB;
};
#define SYSTICK ((struct systick *)0xE000E010)

struct gpio {
    volatile uint32_t MODER, OTYPER, OSPEEDR, PUPDR, IDR, ODR, BSRR, LCKR, AFR[2];
};
#define GPIO(bank) ((struct gpio *) (0x48000000 + 0x400 * (bank)))

struct rcc {
  volatile uint32_t CR, CFGR, CIR, APB2RSTR, APB1RSTR, AHBENR, APB2ENR, APB1ENR,
      BDCR, CSR, AHBRSTR, CFGR2, CFGR3, CR2;
};
#define RCC ((struct rcc *) 0x40021000)

enum { GPIO_MODE_INPUT, GPIO_MODE_OUTPUT, GPIO_MODE_AF, GPIO_MODE_ANALOG };

static inline void gpio_set_mode(uint16_t pin, uint8_t mode) {
  struct gpio *gpio = GPIO(PINBANK(pin));
  int n = PINNO(pin);
  gpio->MODER &= ~(3U << (n * 2));
  gpio->MODER |= (mode & 3) << (n * 2);
}

static inline void gpio_set_pull_up(uint16_t pin) {
    struct gpio *gpio = GPIO(PINBANK(pin));
    int n = PINNO(pin);
    gpio->PUPDR &= ~(3U << (n * 2));   // clear both bits of the field
    gpio->PUPDR |= (1U << (n * 2));
}

static inline void gpio_write(uint16_t pin, bool val) {
  struct gpio *gpio = GPIO(PINBANK(pin));
  gpio->BSRR = (1U << PINNO(pin)) << (val ? 0 : 16);
}

static inline bool gpio_read(uint16_t pin) {
  struct gpio *gpio = GPIO(PINBANK(pin));
  int n = PINNO(pin);
  bool result =  gpio->IDR & (1U << n);
  return result;
}


static inline void systick_init(uint32_t ticks) {
    if ((ticks - 1) > 0xffffff) return;
    SYSTICK->RVR = ticks - 1; //  Cycles are counted until 0, so if we want actual amount of cycles than we should do ticks - 1
    SYSTICK->CVR = 0; // Set Current Value
    SYSTICK->CSR = BIT(0) | BIT(1) | BIT(2); // Turns On, and other configs.
}

static volatile uint32_t s_ticks;
void SysTick_Handler(void) { s_ticks++; }

static inline void delay(uint32_t delay) {
    uint32_t tickstart = s_ticks;
    uint32_t wait = delay;
    if (wait < MAX_DELAY) {
        wait += 1;
    }
    while((s_ticks - tickstart) < wait){}
}

/*
 * SysTick is running
 */
int main(void) {
  uint16_t led = PIN('A', 1); // Pin A5
  uint16_t but1 = PIN('A', 0); // Button Listener
  RCC->AHBENR |= BIT(17 + PINBANK(led)); // Enable GPIO clock for Led
  RCC->AHBENR |= BIT(17 + PINBANK(but1)); // Enable Clock for Button
  systick_init(8000000 / 1000);
  gpio_set_mode(led, GPIO_MODE_OUTPUT); // Set to Output
  gpio_set_mode(but1, GPIO_MODE_INPUT);
  gpio_set_pull_up(but1);
  uint32_t elapsed = 0;
  uint32_t last = s_ticks;
  bool running = true;

  for (;;) {
    bool button = gpio_read(but1);
    uint32_t now = s_ticks;
    uint32_t delta = now - last;
    last = now;

    if (running) {
        elapsed += delta;
    }

    if (elapsed >= 10000) {
        gpio_write(led, true);
    }

    if (button) {
        running = false;
    } else {
        running = true;
  };
  return 0;
}


// Startup Code
__attribute__((naked, noreturn)) void _reset(void) { // naked and noreturn tells compiler that this function won't return anything and pLEASe don't optimise it
  // memset .bss to zero and copy .data section to RAM region
    extern long _sbss, _ebss, _sdata, _edata, _sidata;
    for (long *dst = &_sbss; dst < &_ebss; dst++)
      *dst = 0;
    for (long *dst = &_sdata, *src = &_sidata; dst < &_edata;)
      *dst++ = *src++;

    main();
    for(;;) (void) 0;
}

extern void _estack(void);

__attribute__((section(".vectors"))) void (*const tab[16 + 32])(void) = {
    _estack, _reset, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, SysTick_Handler
};
