#include "board.h"
#include "boot.h"
extern void jump_trampoline(uint32_t sp, uint32_t entry)
    __attribute__((noreturn));
void app_jump(void) {
  __disable_irq();
  SysTick->CTRL = 0;
  SysTick->LOAD = 0;
  SysTick->VAL = 0;
  TIM5->CR1 = 0;
  TIM5->DIER = 0;
  TIM5->SR = 0;
  USART2->CR1 = 0;
  RCC->APB1RSTR |= RCC_APB1RSTR_USART2RST | RCC_APB1RSTR_TIM5RST;
  RCC->APB1RSTR &= ~(RCC_APB1RSTR_USART2RST | RCC_APB1RSTR_TIM5RST);
  for (unsigned i = 0; i < 8; i++) {
    NVIC->ICER[i] = 0xffffffffu;
    NVIC->ICPR[i] = 0xffffffffu;
  }
  SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk | SCB_ICSR_PENDSVCLR_Msk;
  SCB->VTOR = APP_BASE;
  __DSB();
  __ISB();
  jump_trampoline(*(const uint32_t *)APP_BASE,
                  *(const uint32_t *)(APP_BASE + 4));
}
