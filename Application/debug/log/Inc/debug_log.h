#ifndef DEBUG_LOG_H
#define DEBUG_LOG_H

#include <stdarg.h>
#include <stdint.h>
#include "tx_api.h"

// Enabled via -DAPP_DEBUG_LOG (Debug build in .cproject). Disabled in Release.
#if defined(APP_DEBUG_LOG)

// ThreadX tx_queue_create() allows at most TX_16_ULONG (16) ULONGs per message (= 64 B).
#define LOG_MSG_SIZE 64U
#if LOG_MSG_SIZE > 64U
#error "LOG_MSG_SIZE cannot exceed 64 bytes (ThreadX queue message limit)"
#endif
#define LOG_QUEUE_DEPTH 16U
#define LOG_THREAD_STACK_SIZE 2048U
#define LOG_THREAD_PRIORITY 10U

#define DEBUG_LOG_LEVEL_DBG (1U << 0)
#define DEBUG_LOG_LEVEL_INFO (1U << 1)
#define DEBUG_LOG_LEVEL_WARN (1U << 2)
#define DEBUG_LOG_LEVEL_ERROR (1U << 3)
#define DEBUG_LOG_LEVEL_ALL (DEBUG_LOG_LEVEL_DBG | DEBUG_LOG_LEVEL_INFO | \
                             DEBUG_LOG_LEVEL_WARN | DEBUG_LOG_LEVEL_ERROR)

UINT debug_log_init(VOID* memory_ptr);
int debug_log_printf(const char* fmt, ...);
int debug_log_vprintf(const char* fmt, va_list ap);
void debug_log_flush(void);

uint8_t debug_log_level_get_mask(void);
void debug_log_level_set_mask(uint8_t mask);
int debug_log_level_set_by_name(const char* name, int enable);

// Extra detail in app logs (enum names, …). Default on.
void debug_log_verbose_set(int enable);
int debug_log_verbose_get(void);

#define LOGI(fmt, ...)                                         \
    do                                                         \
    {                                                          \
        if (debug_log_level_get_mask() & DEBUG_LOG_LEVEL_INFO) \
            debug_log_printf("[I] " fmt "\n", ##__VA_ARGS__);  \
    } while (0)
#define LOGW(fmt, ...)                                         \
    do                                                         \
    {                                                          \
        if (debug_log_level_get_mask() & DEBUG_LOG_LEVEL_WARN) \
            debug_log_printf("[W] " fmt "\n", ##__VA_ARGS__);  \
    } while (0)
#define LOGE(fmt, ...)                                          \
    do                                                          \
    {                                                           \
        if (debug_log_level_get_mask() & DEBUG_LOG_LEVEL_ERROR) \
            debug_log_printf("[E] " fmt "\n", ##__VA_ARGS__);   \
    } while (0)
#define DBG_LOG(fmt, ...)                                       \
    do                                                          \
    {                                                           \
        if (debug_log_level_get_mask() & DEBUG_LOG_LEVEL_DBG)   \
            debug_log_printf("[DBG] " fmt "\n", ##__VA_ARGS__); \
    } while (0)

#else

UINT debug_log_init(VOID* memory_ptr);
static inline int debug_log_printf(const char* fmt, ...)
{
    (void) fmt;
    return 0;
}
static inline int debug_log_vprintf(const char* fmt, va_list ap)
{
    (void) fmt;
    (void) ap;
    return 0;
}
static inline void debug_log_flush(void)
{
}
static inline void debug_log_verbose_set(int enable)
{
    (void) enable;
}
static inline int debug_log_verbose_get(void)
{
    return 0;
}

#define LOGI(fmt, ...) ((void) 0)
#define LOGW(fmt, ...) ((void) 0)
#define LOGE(fmt, ...) ((void) 0)
#define DBG_LOG(fmt, ...) ((void) 0)

#endif

#endif
