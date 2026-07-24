#include "debug_shell_cmds.h"

#if defined(APP_DEBUG_LOG)

#include "debug_shell.h"
#include "debug_log.h"
#include "debug_uart.h"
#include "firmware_version.h"
#include "system_dump.h"

#include "lwshell/lwshell.h"
#include "lwprintf/lwprintf.h"

#include <stdarg.h>
#include "stm32l4xx.h"

#define SHELL_PRINT_BUF_SIZE 160U

static void shell_printf(const char* fmt, ...)
{
    char buf[SHELL_PRINT_BUF_SIZE];
    va_list ap;

    va_start(ap, fmt);
    (void) lwprintf_vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    debug_shell_print(buf);
}

static int32_t cmd_help(int32_t argc, char** argv)
{
    (void) argc;
    (void) argv;

    debug_shell_print("Commands: help, listcmd, ver, sys, log, reset\r\n");
    debug_shell_print("  listcmd  - list registered commands (LwSHELL)\r\n");
    debug_shell_print("  ver      - firmware version\r\n");
    debug_shell_print("  sys      - ThreadX / memory system dump\r\n");
    debug_shell_print("  log      - levels / verbose (log / log info on / log verbose off)\r\n");
    debug_shell_print("  reset    - MCU reset\r\n");
    debug_shell_print("Enable local echo in your terminal.\r\n");
    return 0;
}

static int32_t cmd_sys(int32_t argc, char** argv)
{
    (void) argc;
    (void) argv;

    system_dump_print();
    return 0;
}

static int32_t cmd_ver(int32_t argc, char** argv)
{
    const firmware_version_info_t* info;

    (void) argc;
    (void) argv;

    info = firmware_version_get();
    shell_printf("%s\r\n", firmware_version_string_full());
    if ((info != NULL) && (info->product_name != NULL))
    {
        shell_printf("product: %s\r\n", info->product_name);
    }
    return 0;
}

static int32_t cmd_log(int32_t argc, char** argv)
{
    uint8_t mask;

    if (argc == 1)
    {
        mask = debug_log_level_get_mask();
        shell_printf("log levels: dbg=%s info=%s warn=%s err=%s\r\n",
                     (mask & DEBUG_LOG_LEVEL_DBG) ? "on" : "off",
                     (mask & DEBUG_LOG_LEVEL_INFO) ? "on" : "off",
                     (mask & DEBUG_LOG_LEVEL_WARN) ? "on" : "off",
                     (mask & DEBUG_LOG_LEVEL_ERROR) ? "on" : "off");
        shell_printf("verbose: %s\r\n", debug_log_verbose_get() ? "on" : "off");
        return 0;
    }

    if (argc != 3)
    {
        debug_shell_print("usage: log [dbg|info|warn|error|all|verbose] [on|off]\r\n");
        return -1;
    }

    {
        int enable = 0;
        if ((argv[2][0] == 'o') && (argv[2][1] == 'n'))
        {
            enable = 1;
        }
        else if ((argv[2][0] == 'o') && (argv[2][1] == 'f'))
        {
            enable = 0;
        }
        else
        {
            debug_shell_print("usage: log <level|verbose> on|off\r\n");
            return -1;
        }

        if ((argv[1][0] == 'v') || (argv[1][0] == 'V'))
        {
            debug_log_verbose_set(enable);
            shell_printf("verbose: %s\r\n", enable ? "on" : "off");
            return 0;
        }

        if (debug_log_level_set_by_name(argv[1], enable) != 0)
        {
            debug_shell_print("unknown level (dbg info warn error all verbose)\r\n");
            return -1;
        }
    }

    return 0;
}

static int32_t cmd_reset(int32_t argc, char** argv)
{
    (void) argc;
    (void) argv;

    debug_shell_print("reset...\r\n");
    debug_uart_flush();
    NVIC_SystemReset();
    return 0;
}

const lwshell_cmd_t debug_shell_static_cmds[] = {
    {cmd_help, "help", "Show command summary"},
    {cmd_ver, "ver", "Firmware version"},
    {cmd_sys, "sys", "ThreadX / memory system dump"},
    {cmd_log, "log", "Log level control"},
    {cmd_reset, "reset", "System reset"},
};

const size_t debug_shell_static_cmds_count = LWSHELL_ARRAYSIZE(debug_shell_static_cmds);

#endif
