#include "debug_shell_cmds.h"

#if defined(APP_DEBUG_LOG)

__attribute__((weak)) int32_t debug_shell_user_hook(int32_t argc, char** argv)
{
    (void) argc;
    (void) argv;
    return 0;
}

#endif
