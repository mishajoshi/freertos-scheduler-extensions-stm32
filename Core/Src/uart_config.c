#include "uart_config.h"
#include <string.h>

static UART_HandleTypeDef *g_huart = NULL;

void uart_init_from_hal(UART_HandleTypeDef *huart)
{
    g_huart = huart;
}

static void uart_blocking_tx(uint8_t *data, uint16_t len)
{
    if (g_huart == NULL) return;   // safety
    HAL_UART_Transmit(g_huart, data, len, HAL_MAX_DELAY);
}

void uart_write_char(uint8_t ch)
{
    uart_blocking_tx(&ch, 1);
}

void uart_write_string(char *str)
{
    uart_blocking_tx((uint8_t *)str, (uint16_t)strlen(str));
}

void uart_write_uint16(uint16_t n)
{
    char buf[8];
    // print as 5-digit decimal (like MSP code)
    snprintf(buf, sizeof(buf), "%05u", (unsigned)n);
    uart_write_string(buf);
    uart_write_string("\r\n");
}

void uart_write_uint32(uint32_t n)
{
    char buf[16];
    // print as up to 10-digit decimal
    snprintf(buf, sizeof(buf), "%lu", (unsigned long)n);
    uart_write_string(buf);
    uart_write_string("\r\n");
}
