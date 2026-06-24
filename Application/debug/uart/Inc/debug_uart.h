#ifndef DEBUG_UART_H
#define DEBUG_UART_H

#include <stdint.h>
#include "tx_api.h"

void debug_uart_putc(uint8_t c);
void debug_uart_puts(const char* s);
void debug_uart_flush(void);

void debug_uart_tx_lock(void);
void debug_uart_tx_unlock(void);
void debug_uart_puts_locked(const char* s);

#if defined(APP_DEBUG_LOG)
UINT debug_uart_tx_mutex_init(void);
#endif

#endif
