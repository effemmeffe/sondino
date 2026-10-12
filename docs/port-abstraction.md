# Astrazione di sondino (multi-RTOS e multi-MCU): note di ripartenza

Data: 12/10/2026. Stato del progetto di evoluzione di sondino, su branch dedicato (`feat/port-abstraction`), separato dal progetto `servo-shell` su ESP32-C6.

## Base e idea

Base: questo repo, pacchetto debug con log, UART e shell (lwshell e lwprintf) per STM32L4 + ThreadX.

Sondino diventa un modulo da importare e configurare una sola volta per RTOS e micro. Nel resto del codice si usano sempre `LOGI/LOGW/LOGE` e la shell. Il modulo gestisce da solo la seriale dedicata.

"Core": la logica indipendente dalla piattaforma (log, shell, gestione UART, formattazione), senza API di RTOS o HAL. Le estensioni (`ext/`) sono i comandi standard (help, ver, log, reset).

## Stato attuale

Il codice e' modulare ma accoppiato a ThreadX e STM32L4:

- `debug_log`: coda ThreadX, thread "log" (prio 10, stack 2048 B), messaggi da 64 B
- `debug_uart`: USART2 e mutex ThreadX per il TX condiviso
- `debug_uart_rx`: ring buffer e hook ISR
- `debug_shell`: thread "shell" (prio 11, stack 1536 B)
- `debug_shell_cmds`: usa `NVIC_SystemReset` e `stm32l4xx.h`
- `system_dump`: il pezzo piu' legato all'RTOS

Gia' portabili: lwshell (verificato), lwprintf (ritenuto portabile, da verificare), logica dei comandi.

## Port layer (`debug_port.h`)

Primitive che il core usa, da implementare per ogni coppia RTOS + MCU:

- thread create e sleep
- mutex (init, lock, unlock)
- coda (init, send, recv)
- UART write, read non bloccante, flush
- reset
- sysdump come estensione per port

Le librerie lw fanno parsing e formattazione; il port sposta solo i byte sulla seriale. Le lw sono C portabile, non legate a STM32.

## Struttura del repo (target)

`core/`, `ext/`, `ports/` (per esempio `threadx_stm32l4/`, `freertos_esp32c6/`), `ThirdParty/`, `docs/`.

## Configurazione

Scelta del port con CMake o Kconfig su ESP-IDF, oppure con un header `debug_port_cfg.h` per STM32CubeIDE. Priorita', stack e dimensione dell'input restano in header di opzioni, come oggi per lwshell.

## Vincoli (da AGENTS.md)

- nessuna allocazione dinamica
- nessun log nelle ISR
- commenti solo `//`
- log solo tramite `LOGx`
- formattazione interna al core con lwprintf
- `LOG_MSG_SIZE` a 64 B per un limite di ThreadX: l'interfaccia della coda va resa generica

## Rischi

- la coda del log e' modellata sui limiti di ThreadX
- `system_dump` deve diventare un'estensione per port
- servono almeno due port reali per validare l'interfaccia (il secondo e' l'ESP)
- evitare regressioni sul codice STM32 usato nel lavoro

## Roadmap

1. Ora: `servo-shell` su ESP32-C6 (FreeRTOS, USB nativa), port scritto nella forma di `debug_port.h`. STM32 non toccato.
2. Poi: migrazione dell'STM32 sul port ThreadX, con comportamento identico verificato.
3. Infine: nuovi port secondo necessita'.

## Decisioni prese

- La shell sta sulla USB nativa.
- `servo-shell` usa `snprintf` della libc come scorciatoia temporanea; nel modulo finale la formattazione e' del core.
- L'astrazione si sviluppa su un branch separato, validata prima dal progetto servo.

## Questioni aperte

- Meccanismo di configurazione: CMake/Kconfig, header, o entrambi
- Forma definitiva della coda (dimensione messaggio e profondita' configurabili)
- Come gestire `sys` sui port senza un equivalente di `system_dump`
- Rendere lwprintf opzionale (acceso di default, spegnibile per usare la libc)
