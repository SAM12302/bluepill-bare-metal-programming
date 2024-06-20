#include <inttypes.h>
#include <stdbool.h>

// Struct Definitions
struct gpio {
  volatile uint32_t CRL, CRH, IDR, ODR, BSRR, BRR, LCKR;
};

struct rcc {
  volatile uint32_t CR, CFGR, CIR, APB2RSTR, APB1RSTR, AHBENR, APB2ENR, APB1ENR, BDCR, CSR;
};

// BIT Definitions
#define BIT(x) (1UL << (x))
#define PIN(bank, num) ((((unsigned)(bank) - 'A') << 8) | (num))
#define PINNO(pin) ((pin) & 255U)
#define PINBANK(pin) ((pin) >> 8)

// Peripheral Base Addresses
#define RCC ((struct rcc *) 0x40021000)
#define GPIO(bank) ((struct gpio *) (0x40010800 + 0x400 * (unsigned)(bank)))
#define GPIOC ((struct gpio *) 0x40011000)

// GPIO Modes
#define GPIO_MODE_INPUT          0x00000000u
#define GPIO_MODE_OUTPUT_PP      0x00000001u
#define GPIO_MODE_OUTPUT_OD      0x00000011u
#define GPIO_MODE_AF_PP          0x00000002u
#define GPIO_MODE_AF_OD          0x00000012u
#define GPIO_MODE_AF_INPUT       GPIO_MODE_INPUT

// GPIO Speed
#define GPIO_SPEED_FREQ_LOW      0x00000001u
#define GPIO_SPEED_FREQ_MEDIUM   0x00000010u
#define GPIO_SPEED_FREQ_HIGH     0x00000011u

// GPIO Pull-up/Pull-down
#define GPIO_NOPULL              0x00000000u
#define GPIO_PULLUP              0x00000001u
#define GPIO_PULLDOWN            0x00000002u

// GPIO Set Mode Functions for Lower and Higher Pins
static inline void gpio_set_mode_Lower(struct gpio *gpio, uint8_t pin, uint8_t mode, uint8_t cnf) {
  gpio->CRL &= ~(0xFu << (pin * 4));        // Clear existing setting
  gpio->CRL |= ((mode & 0x3u) | ((cnf & 0x3u) << 2)) << (pin * 4);   // Set new mode and the new cnf   
}

static inline void gpio_set_mode_Higher(struct gpio *gpio, uint8_t pin, uint8_t mode, uint8_t cnf) {
  gpio->CRH &= ~(0xFu << ((pin - 8) * 4));  // Clear existing setting
  gpio->CRH |= ((mode & 0x3u) | ((cnf & 0x3u) << 2)) << ((pin - 8) * 4);   // Set new mode and the new cnf
}

// Spin Function
static inline void spin(volatile uint32_t count) {
  while (count--) asm("nop");
}

// GPIO Write Function using ODR
static inline void gpio_write(uint16_t pin, bool val) {
  struct gpio *gpio = GPIO(PINBANK(pin));
  if (val) {
    gpio->ODR |= (1U << PINNO(pin));  // Set the pin
  } else {
    gpio->ODR &= ~(1U << PINNO(pin)); // Clear the pin
  }
}

// Main Function
int main(void) {
  uint16_t led = PIN('C', 13);             // Onboard LED connected to PC13
  RCC->APB2ENR |= BIT(4);                  // Enable GPIO clock for GPIOC (bit 4)
  gpio_set_mode_Higher(GPIOC, 13, GPIO_MODE_OUTPUT_PP, GPIO_SPEED_FREQ_LOW);  // Set onboard LED to output mode

  for (;;) {
    gpio_write(led, true);
    spin(999999);
    gpio_write(led, false);
    spin(999999);
  }
  return 0;
}

// Startup code
__attribute__((naked, noreturn)) void _reset(void) {
  // memset .bss to zero, and copy .data section to RAM region
  extern long _sbss, _ebss, _sdata, _edata, _sidata;
  for (long *dst = &_sbss; dst < &_ebss; dst++) *dst = 0;
  for (long *dst = &_sdata, *src = &_sidata; dst < &_edata;) *dst++ = *src++;

  main();             // Call main()
  for (;;) (void) 0;  // Infinite loop in the case if main() returns
}

extern void _estack(void);  // Defined in link.ld

// 16 standard and 91 STM32-specific handlers
__attribute__((section(".vectors"))) void (*const tab[16 + 91])(void) = {
    _estack, _reset};
