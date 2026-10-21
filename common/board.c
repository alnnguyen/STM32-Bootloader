#include "board.h"
#define LED_PIN 5U
/* Both images use reset HSI at 16 MHz, APB1/2 prescalers = 1. */

void board_init(void) {
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOCEN;
  (void)RCC->APB1ENR;
  GPIOA->MODER = (GPIOA->MODER & ~((3u << 4) | (3u << 6) | (3u << 10))) |
                 (2u << 4) | (2u << 6) | (1u << 10);
  GPIOA->AFR[0] =
      (GPIOA->AFR[0] & ~((15u << 8) | (15u << 12))) | (7u << 8) | (7u << 12);
  GPIOA->PUPDR = (GPIOA->PUPDR & ~(3u << 6)) | (1u << 6);
  GPIOC->MODER &= ~(3u << 26);
  GPIOC->PUPDR = (GPIOC->PUPDR & ~(3u << 26)) | (1u << 26);
}

void led_init(void) {
  /* Move your GPIOA clock-enable operation here. */
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
  /* Move your two PA5 MODER operations here. */
  GPIOA->MODER &= ~(3U << (LED_PIN * 2));
  GPIOA->MODER |= (1U << (LED_PIN * 2));
}

void led_toggle(void) {
  /* Move your PA5 ODR toggle operation here. */
  GPIOA->ODR ^= (1U << LED_PIN);
}