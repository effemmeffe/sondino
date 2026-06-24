/**
 ******************************************************************************
 * @file      sysmem.c
 * @brief     Newlib heap (_sbrk) for STM32CubeIDE
 ******************************************************************************
 */

#include "sysmem_stats.h"

#include <errno.h>
#include <stdint.h>

static uint8_t* __sbrk_heap_end = NULL;

void sysmem_get_heap_stats(uintptr_t* heap_used_bytes, uintptr_t* heap_free_bytes, uintptr_t* heap_total_bytes)
{
    extern uint8_t _end;
    extern uint8_t _estack;
    extern uint32_t _Min_Stack_Size;

    const uintptr_t heap_start = (uintptr_t) &_end;
    const uintptr_t stack_limit = (uintptr_t) &_estack - (uintptr_t) &_Min_Stack_Size;
    uintptr_t heap_cur = heap_start;

    if (__sbrk_heap_end != NULL)
    {
        heap_cur = (uintptr_t) __sbrk_heap_end;
    }

    const uintptr_t total = (stack_limit > heap_start) ? (stack_limit - heap_start) : 0U;
    const uintptr_t used = (heap_cur > heap_start) ? (heap_cur - heap_start) : 0U;
    const uintptr_t free = (total > used) ? (total - used) : 0U;

    if (heap_used_bytes != NULL)
    {
        *heap_used_bytes = used;
    }
    if (heap_free_bytes != NULL)
    {
        *heap_free_bytes = free;
    }
    if (heap_total_bytes != NULL)
    {
        *heap_total_bytes = total;
    }
}

void* _sbrk(ptrdiff_t incr)
{
    extern uint8_t _end;
    extern uint8_t _estack;
    extern uint32_t _Min_Stack_Size;
    const uint32_t stack_limit = (uint32_t) &_estack - (uint32_t) &_Min_Stack_Size;
    const uint8_t* max_heap = (uint8_t*) stack_limit;
    uint8_t* prev_heap_end;

    if (NULL == __sbrk_heap_end)
    {
        __sbrk_heap_end = &_end;
    }

    if (__sbrk_heap_end + incr > max_heap)
    {
        errno = ENOMEM;
        return (void*) -1;
    }

    prev_heap_end = __sbrk_heap_end;
    __sbrk_heap_end += incr;

    return (void*) prev_heap_end;
}
