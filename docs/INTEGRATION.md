# Integrating the debug package (log + shell)

Portable stack version: **1.0.0** (core + extensions).

Copy this repository (or tagged subtrees) into an STM32CubeIDE + ThreadX product project.

---

## 1. Package boundary

### Core — always copy

| Path | Role |
|------|------|
| `Application/debug/log/` | ThreadX log queue, `LOGI`/`LOGE`/`LOGW` |
| `Application/debug/uart/` | UART TX/RX for log and shell |
| `Application/debug/shell/` | Shell thread + LwSHELL (registers command table from extensions) |
| `ThirdParty/lwprintf/` | Formatting |
| `ThirdParty/lwshell/` | Shell parser |

### Extensions — recommended

| Path | Role |
|------|------|
| `Application/debug_ext/shell_cmds/` | Commands: `help`, `ver`, `sys`, `log`, `reset` (`listcmd` is built into LwSHELL) |
| `Application/debug_ext/diag/` | `system_dump` for `sys` |
| `Application/product/` | `firmware_version` for `ver` |
| `Application/platform/` | `sysmem.c` heap stats for dump |

### Product-specific (your Cube project)

| Path | Role |
|------|------|
| `Core/` | `MX_USART2_UART_Init`, `USART2_IRQHandler` |
| `AZURE_RTOS/App/` | ThreadX byte pool, `App_ThreadX_Init` |
| Custom commands | New `.c` file with extra `lwshell_cmd_t` table or `lwshell_register_cmd` |

### Not in this package

- Ethernet / `eth_comm` shell commands (`link`, `tcp`, …) — add in product like W5500 `eth_shell_service`
- Full Cube `.ioc` / linker script — stay in product repo

---

## 2. Build symbols (STM32CubeIDE preprocessor)

| Symbol | Debug | Release |
|--------|-------|---------|
| `APP_DEBUG_LOG` | defined | **omit** |
| `DEBUG`, `USE_HAL_DRIVER`, MCU define | per Cube | per Cube |

With `APP_DEBUG_LOG` undefined, `LOGI`/`LOGE` compile to no-ops; shell sources are excluded by `#if`.

---

## 3. Include paths

Add to MCU GCC Compiler → Include paths:

```
../Application/debug/log/Inc
../Application/debug/uart/Inc
../Application/debug/shell/Inc
../Application/debug_ext/shell_cmds/Inc
../Application/debug_ext/diag/Inc
../Application/product/Inc
../Application/platform/Inc
../ThirdParty/lwprintf/lwprintf/src/include
../ThirdParty/lwshell/lwshell/src/include
```

## 4. Source entries

Compile at least:

```
Application/debug/
Application/debug_ext/
Application/product/
Application/platform/
ThirdParty/lwprintf/lwprintf/src/lwprintf
ThirdParty/lwshell/lwshell/src/lwshell
```

---

## 5. Boot sequence

### Before ThreadX (`main`)

```c
MX_GPIO_Init();
MX_USART2_UART_Init();   // or your debug UART
// ...
MX_ThreadX_Init();
```

### `App_ThreadX_Init` (after kernel started)

```c
#include "debug_log.h"
#include "debug_shell.h"

UINT App_ThreadX_Init(VOID *memory_ptr)
{
    UINT ret;

    ret = debug_log_init(memory_ptr);
    if (ret != TX_SUCCESS) {
        debug_uart_puts("debug_log_init failed\r\n");
        return ret;
    }

#if defined(APP_DEBUG_LOG)
    ret = debug_shell_init(memory_ptr);
    if (ret != TX_SUCCESS) {
        LOGW("shell init failed");
    }
#endif

    LOGI("firmware ready");
    // start product threads...
    return TX_SUCCESS;
}
```

Order: **log → shell → application**.

---

## 6. UART ISR hook

In `Core/Src/stm32l4xx_it.c`:

```c
#include "debug_uart_rx.h"

void USART2_IRQHandler(void)
{
#if defined(APP_DEBUG_LOG)
  debug_uart_rx_isr();
#endif
  // HAL_UART_IRQHandler if using HAL for USART2
}
```

Adapt peripheral name if not USART2.

---

## 7. Adapting UART pins / peripheral

Default reference: **USART2** on STM32L476 Nucleo (PA2 TX, PA3 RX), 115200 8N1.

Edit only:

- `Application/debug/uart/Src/debug_uart.c` — TX register access (`USART2`)
- `Application/debug/uart/Src/debug_uart_rx.c` — RX ISR (`USART2`, NVIC)

Log and shell code stay unchanged.

---

## 8. Shell commands (extensions)

| Command | Source |
|---------|--------|
| `help` | `debug_shell_cmds.c` |
| `listcmd` | LwSHELL (`LWSHELL_CFG_USE_LIST_CMD`) |
| `ver` | `firmware_version` + `debug_shell_cmds.c` |
| `sys` | `system_dump.c` |
| `log` | `debug_log` level mask |
| `reset` | `NVIC_SystemReset()` |

### Adding product commands

Option A — edit `debug_shell_static_cmds[]` in a product copy of `debug_ext/shell_cmds/`.

Option B — enable `LWSHELL_CFG_USE_DYNAMIC_COMMANDS 1` and call `lwshell_register_cmd()` after `debug_shell_init`.

---

## 9. Firmware version

1. Set product name and semver in `version.toml`.
2. Run `python scripts/gen_firmware_version.py`.
3. Commit `Application/product/Inc/firmware_version_gen.h`.

Shell: `ver` prints `firmware_version_string_full()`.

---

## 10. Memory / threads

| Item | Size |
|------|------|
| Log thread stack | 2048 B |
| Shell thread stack | 1536 B |
| Log queue | 16 x 64 B messages |

Priorities (ThreadX, lower = more urgent): log **10**, shell **11**.

---

## 11. Import methods

### A — Copy subtree

```bash
git clone <this-repo> && cd stm32-threadx-debug
# Copy Application/debug, Application/debug_ext, Application/product, Application/platform, ThirdParty
```

### B — Git submodule

Submodule at tags `debug-core-v1.0.0` and `debug-ext-v1.0.0`.

---

## 12. Terminal settings

| Setting | Value |
|---------|-------|
| Baud | 115200 8N1 |
| Local echo | **On** |
| Line ending | CR or LF |

---

## Versioning

- `debug-core-vMAJOR.MINOR.PATCH`
- `debug-ext-vMAJOR.MINOR.PATCH`
