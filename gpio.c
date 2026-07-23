/*
 * Every I/O port has 32 bit configuration register
 GPIOA - 0x4800 0000 - 0x4800 03FF
 the offset is 0, the gap is 1024bytes
 */



#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>
#define BIT(x) (1UL << (x))
#define PIN(bank, num) ((((bank) - 'A') << 8) | (num))
#define PINNO(pin) (pin & 255)
#define PINBANK(pin) (pin >> 8)

struct gpio {
    volatile uint32_t MODER, OTYPER, OSPEEDR, PUPDR, IDR, ODR, BSRR, LCKR, AFR[2];
};
#define GPIO(bank) ((struct gpio *) (0x48000000 + 0x400 * (bank)))

struct rcc {
  volatile uint32_t CR, CFGR, CIR, APB2RSTR, APB1RSTR, AHBENR, APB2ENR, APB1ENR,
      BDCR, CSR, AHBRSTR, CFGR2, CFGR3, CR2;
};
#define RCC ((struct rcc *) x4002000)

enum { GPIO_MODE_INPUT, GPIO_MODE_OUTPUT, GPIO_MODE_AF, GPIO_MODE_ANALOG };

// Now we can rewrite previous code as a function
static inline void gpio_set_mode(uint16_t pin, uint8_t mode) {
  struct gpio *gpio = GPIO(PINBANK(pin));
  uint8_t n = PINNO(pin);
  gpio->MODER &= ~(3U << (n * 2));
  gpio->MODER |= (mode & 3) << (n * 2);
}

static inline void gpio_write(uint16_t pin, bool val) {
  struct gpio *gpio = GPIO(PINBANK(pin));
  gpio->BSRR = (1U << PINNO(pin)) << (val ? 0 : 16);
}

static inline void spin(volatile uint32_t count) {
    while(count--) (void) 0;
}


int main() {
  /*
   * Here we assign a MODER register to 0, REGISTER is a memory location in
   * CPU. Pins are actual legs connected to a register. Acording to
   *documentation GPIOA_MODER has 15 pins.
   */
    // *(volatile uint32_t *)(0x48000000 + 0) = 0;
    // *(volatile uint32_t *)(0x48000000 + 0) &= ~(3 << 6); // Clear bit range 6-7
    // *(volatile uint32_t *)(0x48000000 + 0) |= 1 << 6; // Set to 1

  /*
   * Those cryptic writings are not clear and understandable, let's make
   * it more readable. Go to struct
   */
  uint16_t led = PIN('A', 5); // Pin A5
  RCC->AHBENR |= BIT(PINBANK(led)); // Enable GPIO clock for Led
  gpio_set_mode(led, GPIO_MODE_OUTPUT); // Set to Output
  for (;;) {
    gpio_write(led, true);
    spin(999999);
    gpio_write(led, false);
    spin(999999);
  };
  return 0;
}
