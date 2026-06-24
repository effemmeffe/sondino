#ifndef SYSTEM_DUMP_H
#define SYSTEM_DUMP_H

#if defined(APP_DEBUG_LOG)

void system_dump_print(void);

#else

static inline void system_dump_print(void)
{
}

#endif

#endif
