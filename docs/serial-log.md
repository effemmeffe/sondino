# Log seriale e shell

Build **Debug** con `APP_DEBUG_LOG`, terminale **115200 8N1**, local echo **On**.

## Log nel codice

```c
#include "debug_log.h"

LOGI("ready");
LOGI("val=%u", x);
LOGE("spi fail");
LOGW("retry");
DBG_LOG("detail");
```

| Macro | Prefisso |
|-------|----------|
| `LOGI` | `[I]` |
| `LOGW` | `[W]` |
| `LOGE` | `[E]` |
| `DBG_LOG` | `[DBG]` |

- Max **64 caratteri** per messaggio (`LOG_MSG_SIZE`)
- Non usare log in ISR o prima di `debug_log_init()`
- Formattazione altrove: `lwprintf_snprintf` — non `printf` newlib

## Shell (extensions)

Dopo boot: `[shell] ready` → `help`, `listcmd`, `ver`, `sys`, `log`, `reset`.

Vedi [`INTEGRATION.md`](INTEGRATION.md) per integrazione Cube/ThreadX.
