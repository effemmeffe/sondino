# STM32 ThreadX Debug Package

Portable **log + UART + shell** stack for STM32 + ThreadX firmware (reference: NUCLEO-L476, USART2 @ 115200).

Derived from the [W5500](https://github.com/) bench project; split into a reusable template with clear package boundaries.

## Packages

| Package | Path | Tag (planned) | Role |
|---------|------|---------------|------|
| **Core** | `Application/debug/` | `debug-core-v1.0.0` | `debug_log`, `debug_uart`, `debug_shell` (LwSHELL thread, no commands) |
| **Extensions** | `Application/debug_ext/` | `debug-ext-v1.0.0` | Standard shell commands + `system_dump` |
| **Product glue** | `Application/product/`, `Application/platform/` | per project | `firmware_version`, heap stats for `sys` |

ThirdParty: [lwprintf](https://github.com/MaJerle/lwprintf), [lwshell](https://github.com/MaJerle/lwshell).

## Quick start

1. Copy into your STM32CubeIDE + ThreadX project:
   - `Application/debug/`
   - `Application/debug_ext/` (optional but recommended)
   - `Application/product/` + `Application/platform/` (for `ver` / `sys`)
   - `ThirdParty/lwprintf/`, `ThirdParty/lwshell/`
2. Configure USART2 (or adapt `debug_uart.c` / `debug_uart_rx.c`).
3. Add include paths and sources — see [`docs/INTEGRATION.md`](docs/INTEGRATION.md).
4. Define `APP_DEBUG_LOG` in Debug build only.
5. In `App_ThreadX_Init()`: `debug_log_init()` then `debug_shell_init()`.
6. Wire `debug_uart_rx_isr()` in `USART2_IRQHandler` (see integration doc).

Terminal: **115200 8N1**, local echo **On** → `[shell] ready`, then `help`.

## Documentation

- [`docs/INTEGRATION.md`](docs/INTEGRATION.md) — copy list, build symbols, boot sequence, ISR hook
- [`docs/serial-log.md`](docs/serial-log.md) — `LOGI`/`LOGE` usage
- [`AGENTS.md`](AGENTS.md) — AI / contributor context

## Version

Edit `version.toml`, run `python scripts/gen_firmware_version.py`, rebuild.

## License

Application code: project license. ThirdParty: see `ThirdParty/*/LICENSE` or upstream repos.
