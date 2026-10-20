/*
 * Bare-metal LED blink for NUCLEO-F401RE.
 * LD2 (green) is on PA5. No HAL — only CMSIS register definitions.
 * Clock: default 16 MHz HSI (nothing configured).
 */
#include "stm32f4xx.h"

#define LED_PIN 5U

static void delay(volatile uint32_t count) {
  while (count--) {
  }
}

int main(void) {
  /* 1. Enable the clock to GPIOA (RM0368: RCC_AHB1ENR, bit 0) */
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

  /* 2. PA5 as general-purpose output (RM0368: GPIOx_MODER, 2 bits/pin, 01 =
   * output) */
  GPIOA->MODER &= ~(3U << (LED_PIN * 2));
  GPIOA->MODER |= (1U << (LED_PIN * 2));

  /* 3. Toggle PA5 forever (RM0368: GPIOx_ODR) */
  while (1) {
    GPIOA->ODR ^= (1U << LED_PIN);
    delay(500000);
  }
}
