#ifndef DEBUG_UART_RX_H
#define DEBUG_UART_RX_H

#include <stdint.h>
#include <stddef.h>

#if defined(APP_DEBUG_LOG)

void debug_uart_rx_init(void);
void debug_uart_rx_isr(void);
size_t debug_uart_rx_read(uint8_t* buf, size_t max_len);

#else

static inline void debug_uart_rx_init(void)
{
}
static inline void debug_uart_rx_isr(void)
{
}
static inline size_t debug_uart_rx_read(uint8_t* buf, size_t max_len)
{
    (void) buf;
    (void) max_len;
    return 0U;
}

#endif

#endif
