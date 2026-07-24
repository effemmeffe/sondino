#include "system_dump.h"

#if defined(APP_DEBUG_LOG)

#include "debug_uart.h"
#include "firmware_version.h"
#include "sysmem_stats.h"

#include "main.h"
#include "tx_api.h"
#include "tx_block_pool.h"
#include "tx_byte_pool.h"
#include "tx_event_flags.h"
#include "tx_mutex.h"
#include "tx_queue.h"
#include "tx_semaphore.h"
#include "tx_thread.h"
#include "tx_timer.h"

#include "lwprintf/lwprintf.h"

#include <stdarg.h>
#include <string.h>

#define DUMP_LINE_SIZE 192U

static char s_dump_line[DUMP_LINE_SIZE];

// Caller holds debug_uart_tx_lock(); do not use debug_uart_puts_locked() (non-recursive mutex).
static void dump_puts(const char* s)
{
    if (s != NULL)
    {
        debug_uart_puts(s);
    }
}

static void dump_printf(const char* fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    (void) lwprintf_vsnprintf(s_dump_line, sizeof(s_dump_line), fmt, ap);
    va_end(ap);
    dump_puts(s_dump_line);
}

static const char* thread_state_str(UINT state)
{
    switch (state)
    {
        case TX_READY:
            return "READY";
        case TX_COMPLETED:
            return "COMPLETED";
        case TX_TERMINATED:
            return "TERMINATED";
        case TX_SUSPENDED:
            return "SUSPENDED";
        case TX_SLEEP:
            return "SLEEP";
        case TX_QUEUE_SUSP:
            return "QUEUE_SUSP";
        case TX_SEMAPHORE_SUSP:
            return "SEM_SUSP";
        case TX_EVENT_FLAG:
            return "EVENT_FLAG";
        case TX_BLOCK_MEMORY:
            return "BLOCK_MEM";
        case TX_BYTE_MEMORY:
            return "BYTE_MEM";
        case TX_IO_DRIVER:
            return "IO_DRIVER";
        case TX_FILE:
            return "FILE";
        case TX_TCP_IP:
            return "TCP_IP";
        case TX_MUTEX_SUSP:
            return "MUTEX_SUSP";
        case TX_PRIORITY_CHANGE:
            return "PRIO_CHG";
        default:
            return "?";
    }
}

static ULONG thread_stack_used_bytes(const TX_THREAD* thread_ptr)
{
    const UCHAR* ptr;
    ULONG free_bytes = 0U;

    if ((thread_ptr == NULL) || (thread_ptr->tx_thread_stack_start == NULL) ||
        (thread_ptr->tx_thread_stack_size == 0U))
    {
        return 0U;
    }

    ptr = (const UCHAR*) thread_ptr->tx_thread_stack_start;
    while ((free_bytes < thread_ptr->tx_thread_stack_size) && (*ptr == (UCHAR) TX_STACK_FILL))
    {
        ptr++;
        free_bytes++;
    }

    return thread_ptr->tx_thread_stack_size - free_bytes;
}

static void dump_header(void)
{
    const firmware_version_info_t* fw = firmware_version_get();
    ULONG ticks = tx_time_get();
    ULONG tps = TX_TIMER_TICKS_PER_SECOND;
    ULONG sec = (tps != 0U) ? (ticks / tps) : 0U;
    ULONG ms = (tps != 0U) ? ((ticks % tps) * 1000U / tps) : 0U;
    ULONG days = sec / 86400U;
    ULONG h = (sec % 86400U) / 3600U;
    ULONG m = (sec % 3600U) / 60U;
    ULONG s = sec % 60U;
    TX_THREAD* current = tx_thread_identify();

    dump_puts("\r\n========== system dump ==========\r\n");
    if ((fw != NULL) && (fw->version_string_full != NULL))
    {
        dump_printf("firmware: %s (%s)\r\n", fw->product_name, fw->version_string_full);
    }
    dump_printf("build: %s %s\r\n", __DATE__, __TIME__);
    dump_printf("local time: n/a (no RTC); uptime %lud %02lu:%02lu:%02lu.%03lu\r\n",
                (unsigned long) days,
                (unsigned long) h,
                (unsigned long) m,
                (unsigned long) s,
                (unsigned long) ms);
    dump_printf("HAL tick: %lu ms  ThreadX ticks: %lu (%lu/s)\r\n",
                (unsigned long) HAL_GetTick(),
                (unsigned long) ticks,
                (unsigned long) tps);
    dump_printf("current thread: %s\r\n",
                (current != NULL && current->tx_thread_name != NULL) ? current->tx_thread_name : "(none)");
    dump_printf("kernel system_state: %lu\r\n", (unsigned long) _tx_thread_system_state);
}

static void dump_mcu_memory(void)
{
    extern uint32_t _sdata;
    extern uint32_t _edata;
    extern uint32_t _sbss;
    extern uint32_t _ebss;
    extern uint32_t _estack;
    extern uint32_t _end;
    extern uint32_t _Min_Stack_Size;
    uintptr_t heap_used = 0U;
    uintptr_t heap_free = 0U;
    uintptr_t heap_total = 0U;
    const uint32_t ram_size = 96U * 1024U;
    const uint32_t static_used = (uint32_t) &_end - 0x20000000U;

    sysmem_get_heap_stats(&heap_used, &heap_free, &heap_total);

    dump_puts("--- MCU memory ---\r\n");
    dump_printf("SYSCLK: %lu Hz\r\n", (unsigned long) HAL_RCC_GetSysClockFreq());
    dump_printf(".data: %lu B  .bss: %lu B  static+reserve: %lu / %lu B RAM\r\n",
                (unsigned long) ((uint32_t) &_edata - (uint32_t) &_sdata),
                (unsigned long) ((uint32_t) &_ebss - (uint32_t) &_sbss),
                (unsigned long) static_used,
                (unsigned long) ram_size);
    dump_printf("newlib heap: used %lu  free %lu  total %lu B\r\n",
                (unsigned long) heap_used,
                (unsigned long) heap_free,
                (unsigned long) heap_total);
    // Linker absolute symbols: value is the symbol address, not *(addr).
    dump_printf("main stack reserve: %lu B below _estack=0x%08lX\r\n",
                (unsigned long) (uintptr_t) &_Min_Stack_Size,
                (unsigned long) (uintptr_t) &_estack);
}

static void dump_threads(void)
{
    TX_THREAD* thread_ptr;
    ULONG remaining;

    dump_puts("--- threads ---\r\n");
    dump_printf("count: %lu\r\n", (unsigned long) _tx_thread_created_count);

    thread_ptr = _tx_thread_created_ptr;
    remaining = _tx_thread_created_count;
    if ((thread_ptr == NULL) || (remaining == TX_EMPTY))
    {
        dump_puts("(none)\r\n");
        return;
    }

    do
    {
        CHAR* name = NULL;
        UINT state = 0U;
        ULONG run_count = 0U;
        UINT priority = 0U;
        UINT preempt = 0U;
        ULONG time_slice = 0U;
        ULONG stack_used = 0U;
        TX_THREAD* current = tx_thread_identify();

        (void) tx_thread_info_get(thread_ptr, &name, &state, &run_count, &priority, &preempt, &time_slice, TX_NULL, TX_NULL);
        stack_used = thread_stack_used_bytes(thread_ptr);

        dump_printf("  %-12s state=%-11s prio=%u run=%lu stack %lu/%lu B (%lu%%)%s\r\n",
                    (name != NULL) ? name : "?",
                    thread_state_str(state),
                    (unsigned int) priority,
                    (unsigned long) run_count,
                    (unsigned long) stack_used,
                    (unsigned long) thread_ptr->tx_thread_stack_size,
                    (thread_ptr->tx_thread_stack_size != 0U)
                        ? (unsigned long) ((stack_used * 100U) / thread_ptr->tx_thread_stack_size)
                        : 0UL,
                    (thread_ptr == current) ? " *" : "");

        thread_ptr = thread_ptr->tx_thread_created_next;
        remaining--;
    } while ((remaining != 0U) && (thread_ptr != NULL));
}

static void dump_byte_pools(void)
{
    TX_BYTE_POOL* pool_ptr;
    ULONG remaining;

    dump_puts("--- byte pools ---\r\n");
    dump_printf("count: %lu\r\n", (unsigned long) _tx_byte_pool_created_count);

    pool_ptr = _tx_byte_pool_created_ptr;
    remaining = _tx_byte_pool_created_count;
    if ((pool_ptr == NULL) || (remaining == TX_EMPTY))
    {
        dump_puts("(none)\r\n");
        return;
    }

    do
    {
        CHAR* name = NULL;
        ULONG avail = 0U;
        ULONG frags = 0U;
        ULONG suspended = 0U;
        TX_THREAD* first_suspended = NULL;

        (void) tx_byte_pool_info_get(pool_ptr, &name, &avail, &frags, &first_suspended, &suspended, TX_NULL);
        dump_printf("  %-12s size=%lu avail=%lu frag=%lu susp=%lu\r\n",
                    (name != NULL) ? name : "?",
                    (unsigned long) pool_ptr->tx_byte_pool_size,
                    (unsigned long) avail,
                    (unsigned long) frags,
                    (unsigned long) suspended);

        pool_ptr = pool_ptr->tx_byte_pool_created_next;
        remaining--;
    } while ((remaining != 0U) && (pool_ptr != NULL));
}

static void dump_block_pools(void)
{
    TX_BLOCK_POOL* pool_ptr;
    ULONG remaining;

    dump_puts("--- block pools ---\r\n");
    dump_printf("count: %lu\r\n", (unsigned long) _tx_block_pool_created_count);

    pool_ptr = _tx_block_pool_created_ptr;
    remaining = _tx_block_pool_created_count;
    if ((pool_ptr == NULL) || (remaining == TX_EMPTY))
    {
        dump_puts("(none)\r\n");
        return;
    }

    do
    {
        CHAR* name = NULL;
        ULONG avail = 0U;
        ULONG total_blocks = 0U;
        ULONG suspended = 0U;
        TX_THREAD* first_suspended = NULL;

        (void) tx_block_pool_info_get(pool_ptr, &name, &avail, &total_blocks, &first_suspended, &suspended, TX_NULL);
        dump_printf("  %-12s blocks avail=%lu block_size=%u total=%lu susp=%lu\r\n",
                    (name != NULL) ? name : "?",
                    (unsigned long) avail,
                    (unsigned int) pool_ptr->tx_block_pool_block_size,
                    (unsigned long) total_blocks,
                    (unsigned long) suspended);

        pool_ptr = pool_ptr->tx_block_pool_created_next;
        remaining--;
    } while ((remaining != 0U) && (pool_ptr != NULL));
}

static void dump_queues(void)
{
    TX_QUEUE* queue_ptr;
    ULONG remaining;

    dump_puts("--- queues ---\r\n");
    dump_printf("count: %lu\r\n", (unsigned long) _tx_queue_created_count);

    queue_ptr = _tx_queue_created_ptr;
    remaining = _tx_queue_created_count;
    if ((queue_ptr == NULL) || (remaining == TX_EMPTY))
    {
        dump_puts("(none)\r\n");
        return;
    }

    do
    {
        CHAR* name = NULL;
        ULONG enqueued = 0U;
        ULONG storage = 0U;
        ULONG suspended = 0U;
        TX_THREAD* first_suspended = NULL;

        (void) tx_queue_info_get(queue_ptr, &name, &enqueued, &storage, &first_suspended, &suspended, TX_NULL);
        dump_printf("  %-12s msg=%lu/%lu storage=%lu susp=%lu\r\n",
                    (name != NULL) ? name : "?",
                    (unsigned long) enqueued,
                    (unsigned long) queue_ptr->tx_queue_capacity,
                    (unsigned long) storage,
                    (unsigned long) suspended);

        queue_ptr = queue_ptr->tx_queue_created_next;
        remaining--;
    } while ((remaining != 0U) && (queue_ptr != NULL));
}

static void dump_semaphores(void)
{
    TX_SEMAPHORE* sem_ptr;
    ULONG remaining;

    dump_puts("--- semaphores ---\r\n");
    dump_printf("count: %lu\r\n", (unsigned long) _tx_semaphore_created_count);

    sem_ptr = _tx_semaphore_created_ptr;
    remaining = _tx_semaphore_created_count;
    if ((sem_ptr == NULL) || (remaining == TX_EMPTY))
    {
        dump_puts("(none)\r\n");
        return;
    }

    do
    {
        CHAR* name = NULL;
        ULONG value = 0U;
        ULONG suspended = 0U;
        TX_THREAD* first_suspended = NULL;

        (void) tx_semaphore_info_get(sem_ptr, &name, &value, &first_suspended, &suspended, TX_NULL);
        dump_printf("  %-12s count=%lu susp=%lu\r\n",
                    (name != NULL) ? name : "?",
                    (unsigned long) value,
                    (unsigned long) suspended);

        sem_ptr = sem_ptr->tx_semaphore_created_next;
        remaining--;
    } while ((remaining != 0U) && (sem_ptr != NULL));
}

static void dump_mutexes(void)
{
    TX_MUTEX* mutex_ptr;
    ULONG remaining;

    dump_puts("--- mutexes ---\r\n");
    dump_printf("count: %lu\r\n", (unsigned long) _tx_mutex_created_count);

    mutex_ptr = _tx_mutex_created_ptr;
    remaining = _tx_mutex_created_count;
    if ((mutex_ptr == NULL) || (remaining == TX_EMPTY))
    {
        dump_puts("(none)\r\n");
        return;
    }

    do
    {
        CHAR* name = NULL;
        ULONG count = 0U;
        TX_THREAD* owner = NULL;
        ULONG suspended = 0U;
        TX_THREAD* first_suspended = NULL;

        (void) tx_mutex_info_get(mutex_ptr, &name, &count, &owner, &first_suspended, &suspended, TX_NULL);
        dump_printf("  %-12s count=%lu owner=%s susp=%lu\r\n",
                    (name != NULL) ? name : "?",
                    (unsigned long) count,
                    (owner != NULL && owner->tx_thread_name != NULL) ? owner->tx_thread_name : "-",
                    (unsigned long) suspended);

        mutex_ptr = mutex_ptr->tx_mutex_created_next;
        remaining--;
    } while ((remaining != 0U) && (mutex_ptr != NULL));
}

static void dump_event_flags(void)
{
    TX_EVENT_FLAGS_GROUP* group_ptr;
    ULONG remaining;

    dump_puts("--- event flags ---\r\n");
    dump_printf("count: %lu\r\n", (unsigned long) _tx_event_flags_created_count);

    group_ptr = _tx_event_flags_created_ptr;
    remaining = _tx_event_flags_created_count;
    if ((group_ptr == NULL) || (remaining == TX_EMPTY))
    {
        dump_puts("(none)\r\n");
        return;
    }

    do
    {
        CHAR* name = NULL;
        ULONG flags = 0U;
        ULONG suspended = 0U;
        TX_THREAD* first_suspended = NULL;

        (void) tx_event_flags_info_get(group_ptr, &name, &flags, &first_suspended, &suspended, TX_NULL);
        dump_printf("  %-12s flags=0x%08lX susp=%lu\r\n",
                    (name != NULL) ? name : "?",
                    (unsigned long) flags,
                    (unsigned long) suspended);

        group_ptr = group_ptr->tx_event_flags_group_created_next;
        remaining--;
    } while ((remaining != 0U) && (group_ptr != NULL));
}

static void dump_timers(void)
{
    TX_TIMER* timer_ptr;
    ULONG remaining;

    dump_puts("--- timers ---\r\n");
    dump_printf("count: %lu\r\n", (unsigned long) _tx_timer_created_count);

    timer_ptr = _tx_timer_created_ptr;
    remaining = _tx_timer_created_count;
    if ((timer_ptr == NULL) || (remaining == TX_EMPTY))
    {
        dump_puts("(none)\r\n");
        return;
    }

    do
    {
        CHAR* name = NULL;
        UINT active = 0U;
        ULONG remain_ticks = 0U;
        ULONG reschedule = 0U;

        (void) tx_timer_info_get(timer_ptr, &name, &active, &remain_ticks, &reschedule, TX_NULL);
        dump_printf("  %-12s active=%u remain=%lu re_sched=%lu\r\n",
                    (name != NULL) ? name : "?",
                    (unsigned int) active,
                    (unsigned long) remain_ticks,
                    (unsigned long) reschedule);

        timer_ptr = timer_ptr->tx_timer_created_next;
        remaining--;
    } while ((remaining != 0U) && (timer_ptr != NULL));
}

void system_dump_print(void)
{
    debug_uart_tx_lock();
    dump_header();
    dump_mcu_memory();
    dump_threads();
    dump_byte_pools();
    dump_block_pools();
    dump_queues();
    dump_semaphores();
    dump_mutexes();
    dump_event_flags();
    dump_timers();
    dump_puts("========== end dump ==========\r\n\r\n");
    debug_uart_flush();
    debug_uart_tx_unlock();
}

#endif
