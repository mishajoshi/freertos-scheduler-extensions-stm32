#ifndef UART_CONFIG_H_
#define UART_CONFIG_H_

#include "stm32f0xx_hal.h"
#include <stdint.h>

void uart_write_char(uint8_t ch);
void uart_write_string(char *str);
void uart_write_uint16(uint16_t n);
void uart_write_uint32(uint32_t n);
void uart_init_from_hal(UART_HandleTypeDef *huart);

#endif /* UART_CONFIG_H_ */
