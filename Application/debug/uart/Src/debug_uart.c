#include "debug_uart.h"

#include <stddef.h>
#include "stm32l476xx.h"
#include "tx_api.h"

#define DEBUG_UART_TIMEOUT 500000U

#if defined(APP_DEBUG_LOG)
static TX_MUTEX debug_uart_tx_mutex;
static volatile UINT debug_uart_tx_mutex_ready;
#endif

static void debug_uart_wait_txe(void)
{
    uint32_t t = DEBUG_UART_TIMEOUT;

    while ((USART2->ISR & USART_ISR_TXE) == 0U)
    {
        if (--t == 0U)
        {
            return;
        }
    }
}

static void debug_uart_wait_tc(void)
{
    uint32_t t = DEBUG_UART_TIMEOUT;

    while ((USART2->ISR & USART_ISR_TC) == 0U)
    {
        if (--t == 0U)
        {
            return;
        }
    }
}

void debug_uart_putc(uint8_t c)
{
    debug_uart_wait_txe();
    USART2->TDR = (uint32_t) c;
}

void debug_uart_flush(void)
{
    debug_uart_wait_tc();
}

void debug_uart_puts(const char* s)
{
    uint32_t primask = __get_PRIMASK();

    __disable_irq();
    while ((s != NULL) && (*s != '\0'))
    {
        debug_uart_putc((uint8_t) *s++);
    }
    debug_uart_flush();
    __set_PRIMASK(primask);
}

#if defined(APP_DEBUG_LOG)

void debug_uart_tx_lock(void)
{
    if (debug_uart_tx_mutex_ready != 0U)
    {
        (void) tx_mutex_get(&debug_uart_tx_mutex, TX_WAIT_FOREVER);
    }
}

void debug_uart_tx_unlock(void)
{
    if (debug_uart_tx_mutex_ready != 0U)
    {
        (void) tx_mutex_put(&debug_uart_tx_mutex);
    }
}

void debug_uart_puts_locked(const char* s)
{
    debug_uart_tx_lock();
    debug_uart_puts(s);
    debug_uart_tx_unlock();
}

UINT debug_uart_tx_mutex_init(void)
{
    UINT status;

    status = tx_mutex_create(&debug_uart_tx_mutex, "uart_tx", TX_INHERIT);
    if (status == TX_SUCCESS)
    {
        debug_uart_tx_mutex_ready = 1U;
    }
    return status;
}

#else

void debug_uart_tx_lock(void)
{
}
void debug_uart_tx_unlock(void)
{
}
void debug_uart_puts_locked(const char* s)
{
    debug_uart_puts(s);
}

#endif
