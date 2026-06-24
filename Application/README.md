# Application/

| Area | Path | Package |
|------|------|---------|
| Debug core | `debug/` | Portable — log, UART, shell thread |
| Debug extensions | `debug_ext/` | Optional — shell commands, system dump |
| Product | `product/` | Per firmware — version |
| Platform | `platform/` | Per firmware — heap (`sysmem`) |

Integration guide: [`docs/INTEGRATION.md`](../docs/INTEGRATION.md).
