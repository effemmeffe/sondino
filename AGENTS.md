# AI Context — stm32-threadx-debug

## Repository role

Portable **debug core** (log + UART + shell infrastructure) + **extensions** (standard shell commands, system dump).  
Copy into STM32 + ThreadX product firmware; not a flashable Cube project by itself.

## Stack

- MCU: STM32L4 family (reference: STM32L476RG, NUCLEO-L476RG)
- RTOS: ThreadX (Azure RTOS)
- Print/log: lwprintf + macro LOGI/LOGE/LOGW (`debug_log`)
- Shell: lwshell (`debug_shell`)
- Toolchain: arm-none-eabi-gcc, STM32CubeIDE

## Package boundary

### Core (`Application/debug/`)

| Module | Path | Note |
|--------|------|------|
| debug_log | debug/log/ | ThreadX queue + thread "log", lwprintf |
| debug_uart | debug/uart/ | USART2 TX (reference), mutex for log/shell |
| debug_uart_rx | debug/uart/ | RX ring + ISR hook |
| debug_shell | debug/shell/ | Thread "shell", LwSHELL; **no commands** |

### Extensions (`Application/debug_ext/`)

| Module | Path | Note |
|--------|------|------|
| debug_shell_cmds | debug_ext/shell_cmds/ | help, ver, sys, log, reset; listcmd = LwSHELL built-in |
| system_dump | debug_ext/diag/ | ThreadX / heap dump for `sys` |

### Product (per firmware)

| Module | Path | Note |
|--------|------|------|
| firmware_version | product/ | `ver` command; `version.toml` + gen script |
| sysmem | platform/ | Heap stats for `system_dump` |

## Vincoli

- No dynamic allocation in application debug code
- ISR: only `debug_uart_rx_isr()` from USART IRQ; no log in ISR
- Comments: `//` only
- Log: LOGI/LOGE/LOGW — never direct `lwprintf_printf` in app code
- String format: `lwprintf_snprintf` / `lwprintf_vsnprintf`
- `LOG_MSG_SIZE` max **64 B** (ThreadX `TX_16_ULONG` queue limit)
- Build debug: `APP_DEBUG_LOG` enables log, shell, extensions

## Thread priorities (lower = more urgent)

| Task | Name | Prio | Stack |
|------|------|------|-------|
| debug_log | "log" | 10 | 2048 B |
| debug_shell | "shell" | 11 | 1536 B |

## Build symbols

- `APP_DEBUG_LOG`: Debug configuration only
- `DEBUG`, `USE_HAL_DRIVER`, `STM32L476xx` (or your L4 part): Cube defaults

## Periferiche (reference Nucleo-L476)

- USART2 PA2/PA3: debug UART — log + shell
- Call `debug_uart_rx_isr()` from `USART2_IRQHandler` in `stm32l4xx_it.c`

## Integrazione

See `docs/INTEGRATION.md`.
