#ifndef DEBUG_SHELL_H
#define DEBUG_SHELL_H

#include "tx_api.h"

#if defined(APP_DEBUG_LOG)

UINT debug_shell_init(VOID* memory_ptr);
void debug_shell_print(const char* str);

#else

static inline UINT debug_shell_init(VOID* memory_ptr)
{
    (void) memory_ptr;
    return TX_SUCCESS;
}

static inline void debug_shell_print(const char* str)
{
    (void) str;
}

#endif

#endif
