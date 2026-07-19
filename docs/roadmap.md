# Embedded Kernel — Implementation Roadmap

11 layers. Each one is a prerequisite for the next. "Done" means you can demonstrate
the interface it exposes upward, not just that the code compiles.

## Key references — download and keep these open

- **RM0383** — STM32F411xC/E Reference Manual (all peripherals, all registers)
  https://www.st.com/resource/en/reference_manual/rm0383-stm32f411xce-advanced-armbased-32bit-mcus-stmicroelectronics.pdf

- **PM0214** — STM32F4 Cortex-M4 Programming Manual (core, NVIC, MPU, exceptions)
  https://www.st.com/resource/en/programming_manual/pm0214-stm32-cortexm4-mcus-and-mpus-programming-manual-stmicroelectronics.pdf

- **DS10314** — STM32F411 Datasheet (pinout, alternate function table, electrical specs)
  https://www.st.com/resource/en/datasheet/stm32f411ce.pdf

- **ARMv7-M Architecture Reference Manual** — authoritative on ISA, exception model,
  privilege levels, MPU (goes deeper than PM0214)
  https://developer.arm.com/documentation/ddi0403/latest/

---

```
┌─────────────────────────────────────────────────────┐
│  L11  APPLICATION      TUI, menu, modes, UX         │
├─────────────────────────────────────────────────────┤
│  L10  PROTOCOL         imgproto on-device, image rx │
├─────────────────────────────────────────────────────┤
│  L9   NETWORKING       W5500, socket abstraction    │
├─────────────────────────────────────────────────────┤
│  L8   OS SERVICES      allocator, storage, tasks    │
├─────────────────────────────────────────────────────┤
│  L7   KERNEL           MPU, privilege, SVCall       │
├─────────────────────────────────────────────────────┤
│  L6   DEVICE DRIVERS   ST7735, W5500 HAL            │
├─────────────────────────────────────────────────────┤
│  L5   BUS + DMA        SPI, DMA channels            │
├─────────────────────────────────────────────────────┤
│  L4   INTERRUPTS       NVIC, SysTick, vector table  │
├─────────────────────────────────────────────────────┤
│  L3   UART             debug channel, first periph  │
├─────────────────────────────────────────────────────┤
│  L2   CLOCKS           RCC, PLL, clock tree         │
├─────────────────────────────────────────────────────┤
│  L1   BOOT             linker, startup, memory map  │
└─────────────────────────────────────────────────────┘
```

---

## L1 — Boot
**Implement:** Linker script and startup.c — written from scratch, not copied.
Full vector table (or at minimum a correctly structured one that can be extended).
`.data`/`.bss` init. `.init_array` loop for C++ global constructors.

**NOTE:** The existing `startup.c` is a minimal blink stub (4-entry vector table,
no C++ support). It gets moved to `firmware/src/` during reorganization but is
treated as a blank slate — the L1 task IS the rewrite.

**Done when:** You can write both files cold without reference. A global C++ object
provably gets constructed before `main()`. You can explain every symbol in the
linker script.

**Exposes upward:** A known, initialized memory map. Everything above assumes this.

**Read:**
- PM0214 §2.3 — Exception model
- PM0214 §2.3.4 — Vector table structure, what hardware reads on reset
- RM0383 §2.1 — Memory map (Flash at 0x08000000, SRAM at 0x20000000)
- RM0383 §2.4 — Boot modes (BOOT0 pin, address aliasing)
- GCC docs on linker scripts: https://sourceware.org/binutils/docs/ld/Scripts.html

---

## L2 — Clocks
**Implement:** RCC PLL configuration — take HSI (16 MHz) to 100 MHz system clock.
Configure AHB, APB1 (max 50 MHz), APB2 (max 100 MHz) prescalers correctly.
Verify SYSCLK via SysTick period.

**Done when:** SysTick fires at a known, correct interval. You can derive UART baud
rates and SPI clock speeds from the configured tree without guessing.

**Exposes upward:** A known system clock. Every peripheral timing calculation
downstream depends on this being correct.

**Read:**
- RM0383 §6.2 — Clock tree diagram (read this first, it's a diagram not text)
- RM0383 §6.3.2 — PLL configuration (PLLM, PLLN, PLLP formula)
- RM0383 §6.3.3 — RCC_CR register
- RM0383 §6.3.10 — RCC_CFGR (clock source select, prescalers)

---

## L3 — UART
**Implement:** USART on correct pins with correct alternate function configured
on the GPIO. Baud rate via BRR register derived from SYSCLK. TX/RX. C++ `Uart`
class wrapping it.

**Done when:** `uart.write("hello\n")` appears in a serial terminal. You are no
longer blind for every layer that follows.

**Exposes upward:** A debug channel. Every layer above is built and debugged
through this — do not skip it.

**Read:**
- RM0383 §19 — USART (baud rate calculation is §19.3.4, BRR register §19.6.3)
- DS10314 Table 9 — Alternate function mapping (which pin maps to which USART)
- RM0383 §8 — GPIO (alternate function config, MODER/AFRL/AFRH registers)

---

## L4 — Interrupts
**Implement:** Full 98-entry vector table (all slots, weak aliases for unused ones).
NVIC priority grouping. SysTick handler → millisecond tick. Understand the full
exception stack frame — what hardware pushes automatically, what EXC_RETURN means.

**Done when:** `millis()` works. `delay_ms()` is interrupt-driven, not a spin loop.
You can describe the stack frame at the moment an interrupt fires.

**Exposes upward:** Async event handling. DMA completion, UART RX, SPI done —
everything above uses interrupts.

**Read:**
- PM0214 §2.3 — Exception model (the full picture)
- PM0214 §2.3.7 — Exception entry/exit, stack frame layout, EXC_RETURN values
- PM0214 §4.3 — NVIC registers and priority configuration
- RM0383 Table 37 — Full STM32F411 vector table (all 98 entries with IRQ numbers)

---

## L5 — Bus + DMA
**Implement:** SPI1 peripheral in master mode. SCK/MOSI/MISO/NSS on correct pins
with AF5. Then DMA2 Stream3 Channel3 for SPI1 TX — non-blocking transfers with
transfer-complete interrupt.

**Done when:** Arbitrary buffer sent to an SPI device via DMA. CPU is provably free
during transfer (measure with a GPIO toggle). Completion interrupt fires correctly.

**Exposes upward:** A fast, non-blocking byte pipe. Both the display driver and
W5500 driver are built on this.

**Read:**
- RM0383 §20 — SPI (control registers §20.5, CR1/CR2/SR/DR)
- RM0383 §9 — DMA controller
- RM0383 Table 28 — DMA request mapping (SPI1_TX = DMA2 Stream3 Ch3, not configurable)
- RM0383 §20.5.22 — SPI + DMA procedure

---

## L6 — Device Drivers
**Implement:** ST7735 initialization sequence (vendor command list over SPI),
pixel write, rectangle fill, text rendering. Later: W5500 register-level driver.

**Done when:** Arbitrary pixels and text on display. Driver exposes
`draw_pixel(x, y, color)` / `draw_text(x, y, str)` — nothing above touches SPI.

**Exposes upward:** Abstract display surface and (later) socket interface.

**Read:**
- ST7735S datasheet (Sitronix) — init sequence is in §8, command set in §9
  Search: "ST7735S datasheet Sitronix" — multiple panel vendors use this controller
- W5500 datasheet: https://docs.wiznet.io/Product/iEthernet/W5500/datasheet

---

## L7 — Kernel
**Implement:** MPU region configuration — flash RX, SRAM RW/NX, stack guard page.
Drop to Unprivileged Thread mode after boot. SVCall handler as kernel entry point.
Minimal syscall dispatch table. Stack pointer randomization at boot.

**Done when:** Code in Unprivileged mode cannot write kernel memory (MPU raises
MemManage fault). Kernel entry only via SVCall. Stack start varies across boots.

**Exposes upward:** A security boundary. Application runs unprivileged and goes
through a defined syscall interface.

**Read:**
- PM0214 §2.1.3 — Privileged and unprivileged software
- PM0214 §2.3.7 — SVC (SVCall) exception
- PM0214 §4.5 — MPU registers and configuration
- ARMv7-M ARM §B3.5 — MPU (more detailed than PM0214)
- ARM AppNote: https://developer.arm.com/documentation/ka004046/latest (MPU usage)

---

## L8 — OS Services
**Implement:** Pool allocator (fixed-size blocks, no heap fragmentation, no malloc).
Flash storage abstraction over internal flash sectors or SD card via SPI.
Optional: minimal cooperative scheduler if concurrency is needed.

**Done when:** Higher layers can allocate fixed-size buffers, persist images to
storage, and retrieve them by index.

**Exposes upward:** Memory and storage primitives.

**Read:**
- RM0383 §2.6 — Embedded Flash interface
- RM0383 §2.6.7 — Flash erase and program sequences (you must erase before write)

---

## L9 — Networking
**Implement:** W5500 SPI driver (register-level, on top of L5). Thin
BSD-socket-like abstraction. TCP listen on a port, accept connection, receive bytes.

**Done when:** Raw byte stream receivable from a PC over TCP on a known port.

**Exposes upward:** A socket delivering a byte stream to L10.

**Note:** Can prototype L10 over UART first, then swap in real networking — the
protocol layer shouldn't care which transport delivers the bytes.

**Read:**
- W5500 datasheet (full register map): https://docs.wiznet.io/Product/iEthernet/W5500/datasheet
- W5500 application note (TCP server example): https://docs.wiznet.io/Product/iEthernet/W5500

---

## L10 — Protocol
**Implement:** Port `shared/imgproto_wire.h` packet parser to run on-device.
Validate magic bytes, parse header fields, reassemble chunks, apply color mode
transform, hand decoded image to L6 display driver.

**Done when:** Host-side sender transmits an image, it appears on the ST7735.

**Exposes upward:** A decoded `Image` — application layer receives images, never
sees packets or chunks.

---

## L11 — Application
**Implement:** TUI on 128×128 (retro terminal aesthetic), button input via GPIO
interrupts, state machine: IDLE → LISTEN / LOAD / GENERATE / SHUTDOWN.
Encryption at the protocol boundary. Wireshark dissector on the host side.

**Done when:** Full vision running with memory protection enforced underneath.

**Read (when you get here):**
- Wireshark dissector API: https://wiki.wireshark.org/Lua/Dissectors (Lua) or
  https://www.wireshark.org/docs/wsdg_html_chunked/ChDissectAdd.html (C plugin)
