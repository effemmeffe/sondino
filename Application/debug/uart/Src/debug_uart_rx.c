#include "debug_uart_rx.h"

#if defined(APP_DEBUG_LOG)

#include "stm32l476xx.h"
#include "main.h"
#include "stm32l4xx_hal.h"

#define DEBUG_UART_RX_RING_SIZE 256U

static uint8_t s_rx_ring[DEBUG_UART_RX_RING_SIZE];
static volatile uint16_t s_rx_head;
static volatile uint16_t s_rx_tail;

static void debug_uart_rx_push(uint8_t byte)
{
    uint16_t next = (uint16_t) ((s_rx_head + 1U) % DEBUG_UART_RX_RING_SIZE);

    if (next == s_rx_tail)
    {
        return;
    }
    s_rx_ring[s_rx_head] = byte;
    s_rx_head = next;
}

void debug_uart_rx_init(void)
{
    s_rx_head = 0U;
    s_rx_tail = 0U;
    USART2->CR1 |= USART_CR1_RXNEIE;
    HAL_NVIC_SetPriority(USART2_IRQn, 7, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
}

void debug_uart_rx_isr(void)
{
    if ((USART2->ISR & USART_ISR_RXNE) != 0U)
    {
        debug_uart_rx_push((uint8_t) USART2->RDR);
    }
    if ((USART2->ISR & USART_ISR_ORE) != 0U)
    {
        USART2->ICR = USART_ICR_ORECF;
    }
}

size_t debug_uart_rx_read(uint8_t* buf, size_t max_len)
{
    size_t count = 0U;

    if ((buf == NULL) || (max_len == 0U))
    {
        return 0U;
    }

    while ((count < max_len) && (s_rx_tail != s_rx_head))
    {
        buf[count++] = s_rx_ring[s_rx_tail];
        s_rx_tail = (uint16_t) ((s_rx_tail + 1U) % DEBUG_UART_RX_RING_SIZE);
    }

    return count;
}

#endif
