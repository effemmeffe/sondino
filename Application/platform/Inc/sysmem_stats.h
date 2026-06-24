#ifndef SYSMEM_STATS_H
#define SYSMEM_STATS_H

#include <stdint.h>

void sysmem_get_heap_stats(uintptr_t* heap_used_bytes, uintptr_t* heap_free_bytes, uintptr_t* heap_total_bytes);

#endif
