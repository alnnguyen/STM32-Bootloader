#ifndef BOARD_H
#define BOARD_H
#include "stm32f4xx.h"
#include <stdbool.h>
void board_init(void);

void led_init(void);
void led_toggle(void);

#endif