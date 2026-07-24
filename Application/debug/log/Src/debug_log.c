#include "debug_log.h"
#include "debug_uart.h"

#if defined(APP_DEBUG_LOG)

#include "lwprintf/lwprintf.h"

#include <stdarg.h>
#include <stdint.h>

typedef struct
{
    char text[LOG_MSG_SIZE];
} log_msg_t;

typedef char log_msg_ulong_align_check[(sizeof(log_msg_t) % sizeof(ULONG)) == 0U ? 1 : -1];

#define LOG_MSG_ULONGS (sizeof(log_msg_t) / sizeof(ULONG))

static TX_QUEUE log_queue;
static TX_THREAD log_thread;
static ULONG log_queue_storage[LOG_QUEUE_DEPTH * LOG_MSG_ULONGS];
static UCHAR log_thread_stack[LOG_THREAD_STACK_SIZE];
static volatile UINT log_dropped_count;
static volatile uint8_t log_level_mask = DEBUG_LOG_LEVEL_ALL;

static void log_uart_puts_crlf(const char* s)
{
    while ((s != NULL) && (*s != '\0'))
    {
        if (*s == '\n')
        {
            debug_uart_putc('\r');
            debug_uart_putc('\n');
        }
        else
        {
            debug_uart_putc((uint8_t) *s);
        }
        s++;
    }
    debug_uart_flush();
}

static void log_msg_finalize(log_msg_t* msg, int formatted_len)
{
    size_t len = 0U;
    size_t cap = sizeof(msg->text);

    if (msg == NULL)
    {
        return;
    }

    while ((len < cap) && (msg->text[len] != '\0'))
    {
        len++;
    }

    if ((formatted_len >= (int) (cap - 1U)) || (len >= (cap - 1U)))
    {
        if (cap >= 6U)
        {
            msg->text[cap - 6U] = '.';
            msg->text[cap - 5U] = '.';
            msg->text[cap - 4U] = '.';
            msg->text[cap - 3U] = '\n';
            msg->text[cap - 2U] = '\0';
        }
        return;
    }

    if ((len == 0U) || (msg->text[len - 1U] != '\n'))
    {
        if (len + 1U < cap)
        {
            msg->text[len] = '\n';
            msg->text[len + 1U] = '\0';
        }
    }
}

static void log_thread_entry(ULONG input)
{
    log_msg_t msg;

    (void) input;

    for (;;)
    {
        if (tx_queue_receive(&log_queue, &msg, TX_WAIT_FOREVER) != TX_SUCCESS)
        {
            continue;
        }
        debug_uart_tx_lock();
        log_uart_puts_crlf(msg.text);
        debug_uart_tx_unlock();
    }
}

int debug_log_vprintf(const char* fmt, va_list ap)
{
    log_msg_t msg;
    int formatted_len;

    if (fmt == NULL)
    {
        return -1;
    }

    formatted_len = lwprintf_vsnprintf(msg.text, sizeof(msg.text), fmt, ap);
    log_msg_finalize(&msg, formatted_len);

    if (tx_queue_send(&log_queue, &msg, TX_NO_WAIT) != TX_SUCCESS)
    {
        log_dropped_count++;
        return -1;
    }

    return 0;
}

int debug_log_printf(const char* fmt, ...)
{
    va_list ap;
    int ret;

    va_start(ap, fmt);
    ret = debug_log_vprintf(fmt, ap);
    va_end(ap);

    return ret;
}

uint8_t debug_log_level_get_mask(void)
{
    return log_level_mask;
}

void debug_log_level_set_mask(uint8_t mask)
{
    log_level_mask = mask & DEBUG_LOG_LEVEL_ALL;
}

int debug_log_level_set_by_name(const char* name, int enable)
{
    uint8_t bit = 0U;

    if (name == NULL)
    {
        return -1;
    }
    if ((name[0] == 'd') || (name[0] == 'D'))
    {
        bit = DEBUG_LOG_LEVEL_DBG;
    }
    else if ((name[0] == 'i') || (name[0] == 'I'))
    {
        bit = DEBUG_LOG_LEVEL_INFO;
    }
    else if ((name[0] == 'w') || (name[0] == 'W'))
    {
        bit = DEBUG_LOG_LEVEL_WARN;
    }
    else if ((name[0] == 'e') || (name[0] == 'E'))
    {
        bit = DEBUG_LOG_LEVEL_ERROR;
    }
    else if ((name[0] == 'a') || (name[0] == 'A'))
    {
        if (enable != 0)
        {
            log_level_mask = DEBUG_LOG_LEVEL_ALL;
        }
        else
        {
            log_level_mask = 0U;
        }
        return 0;
    }
    else
    {
        return -1;
    }

    if (enable != 0)
    {
        log_level_mask |= bit;
    }
    else
    {
        log_level_mask &= (uint8_t) ~bit;
    }
    return 0;
}

void debug_log_flush(void)
{
    ULONG enqueued = 1U;
    UINT stable_empty = 0U;

    while (stable_empty < 3U)
    {
        (void) tx_queue_info_get(&log_queue, TX_NULL, &enqueued, TX_NULL, TX_NULL, TX_NULL, TX_NULL);
        if (enqueued == 0U)
        {
            stable_empty++;
            (void) tx_thread_sleep(2U);
        }
        else
        {
            stable_empty = 0U;
            (void) tx_thread_sleep(1U);
        }
    }
}

UINT debug_log_init(VOID* memory_ptr)
{
    UINT status;

    (void) memory_ptr;

    if (debug_uart_tx_mutex_init() != TX_SUCCESS)
    {
        return TX_NOT_AVAILABLE;
    }

    status = tx_queue_create(&log_queue, "log_q", LOG_MSG_ULONGS, log_queue_storage, sizeof(log_queue_storage));
    if (status != TX_SUCCESS)
    {
        return status;
    }

    status = tx_thread_create(&log_thread, "log", log_thread_entry, 0U, log_thread_stack, sizeof(log_thread_stack), LOG_THREAD_PRIORITY, LOG_THREAD_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START);
    if (status != TX_SUCCESS)
    {
        return status;
    }

    return TX_SUCCESS;
}

#else

UINT debug_log_init(VOID* memory_ptr)
{
    (void) memory_ptr;
    return TX_SUCCESS;
}

void debug_log_flush(void)
{
}

#endif

int __io_putchar(int ch)
{
    uint8_t c = (uint8_t) ch;

    if (c == '\n')
    {
        debug_uart_putc('\r');
        debug_uart_putc('\n');
        debug_uart_flush();
    }
    else
    {
        debug_uart_putc(c);
    }
    return ch;
}
