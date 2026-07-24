#include "debug_shell.h"

#if defined(APP_DEBUG_LOG)

#include "debug_uart.h"
#include "debug_uart_rx.h"
#include "lwshell/lwshell.h"

#include <stddef.h>
#include <stdint.h>

#define SHELL_THREAD_STACK_SIZE 1536U
#define SHELL_THREAD_PRIORITY 11U

static TX_THREAD shell_thread;
static UCHAR shell_thread_stack[SHELL_THREAD_STACK_SIZE];

extern const lwshell_cmd_t debug_shell_static_cmds[];
extern const size_t debug_shell_static_cmds_count;

void debug_shell_print(const char* str)
{
    if (str != NULL)
    {
        debug_uart_puts_locked(str);
    }
}

void debug_shell_print_ready(void)
{
    debug_shell_print("\r\n[shell] ready. Type 'help' + Enter\r\n");
}

static void shell_output_fn(const char* str, lwshell_t* lwobj)
{
    (void) lwobj;
    debug_shell_print(str);
}

static void shell_thread_entry(ULONG input)
{
    uint8_t rx_buf[32];
    size_t n;
    size_t i;

    (void) input;

    for (;;)
    {
        n = debug_uart_rx_read(rx_buf, sizeof(rx_buf));
        for (i = 0U; i < n; i++)
        {
            (void) lwshell_input(&rx_buf[i], 1U);
        }
        if (n == 0U)
        {
            tx_thread_sleep(1U);
        }
    }
}

UINT debug_shell_init(VOID* memory_ptr)
{
    UINT status;

    (void) memory_ptr;

    debug_uart_rx_init();

    if (lwshell_init() != lwshellOK)
    {
        return TX_NOT_AVAILABLE;
    }

    if (lwshell_set_output_fn(shell_output_fn) != lwshellOK)
    {
        return TX_NOT_AVAILABLE;
    }

    if (lwshell_register_static_cmds(debug_shell_static_cmds, debug_shell_static_cmds_count) != lwshellOK)
    {
        return TX_NOT_AVAILABLE;
    }

    status = tx_thread_create(&shell_thread, "shell", shell_thread_entry, 0U, shell_thread_stack, sizeof(shell_thread_stack), SHELL_THREAD_PRIORITY, SHELL_THREAD_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START);
    return status;
}

#endif
