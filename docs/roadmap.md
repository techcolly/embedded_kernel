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

---

## Later — Port to a different architecture

A while from now this gets ported to another architecture. Nothing needs to happen
about it yet, but it's worth knowing which layers survive the move and which don't.

**Rewritten entirely:** L1 (linker script, startup, vector table), L2 (RCC/PLL),
L4 (NVIC, exception model), L7 (MPU, privilege, syscall entry) — these are
STM32/Cortex-M specific end to end.

**Rewritten at the register level, same shape:** L3 (UART), L5 (SPI + DMA). The
class interfaces stay; the register writes behind them are all new.

**Portable as-is:** L6 upward. ST7735R and W5500 are external chips — their command
sequences and register maps don't change with the host CPU. L8/L9/L10/L11 are
logic, not hardware.

The practical consequence: the more that hardware access stays behind the `Uart` /
`Spi` class boundaries and out of the upper layers, the smaller the port. Direct
register pokes above L5 are what make it expensive.

---

# Fork — RTOS branch (drone flight controller)

A parallel project in progress elsewhere. It reuses L1–L5 as a base and then
diverges hard: this branch is **hard real time**, where the image OS is soft real
time. Missing a deadline here isn't a dropped frame, it's an aircraft falling out
of the sky.

```
                        R12  TELEMETRY / BLACKBOX
                        R11  SAFETY + FAILSAFE
                        R10  CONTROL + ACTUATION
                        R9   SENSOR PIPELINE
                        R8   SYNCHRONIZATION
                        R7   PREEMPTIVE KERNEL
                        R6   TIMING SUBSTRATE
                              ▲
        ┌─────────────────────┴─────────────────────┐
   L6–L11 image OS                            R6–R12 flight controller
        └─────────────────────┬─────────────────────┘
                              │
                      L1–L5   SHARED BASE
                      boot, clocks, UART, NVIC, SPI+DMA
```

## What the fork costs in L1–L5

The shared base isn't free — three things need retrofitting before R6 is viable.

**L4 needs a deliberate interrupt priority map.** Everything currently runs at the
default priority, which is fine when nothing is time-critical. Here the map is
load-bearing: IMU data-ready at the top, then the control loop, then comms, with
logging at the bottom. PRIGROUP is already set to 3 in `Reset_Handler`.

**Critical sections must use BASEPRI, not `cpsid i`.** Disabling all interrupts
blocks the gyro ISR too. Writing a priority threshold to BASEPRI masks only what's
at or below it, so the highest-priority handlers stay live through a critical
section. This is the single most important habit difference from the image OS.

**L5 needs bus arbitration.** One SPI bus, several devices (IMU, baro, mag, OSD,
flash), tasks at different priorities all wanting it. Either a mutex with priority
inheritance, or a single bus-owner task fed by a request queue. The current
`Spi` class assumes one caller and one device per port.

Everything else in L1–L5 carries over intact — and the async transport design pays
off immediately, since `spi__transfer()` already returns without blocking.

---

## R6 — Timing Substrate
**Implement:** A free-running 32-bit hardware timer at 1 MHz for microsecond
timestamps (TIM2 and TIM5 are the 32-bit ones on F411 — TIM3/TIM4 are 16-bit and
wrap every 65 ms at 1 MHz, which is useless here). `micros()` alongside the
existing `millis()`. Jitter instrumentation: timestamp loop entry, track
min/max/stddev of the delta.

**Done when:** You can measure your own loop jitter and state it in microseconds.
Timestamps don't glitch across the 32-bit wrap.

**Exposes upward:** A time base precise enough that control-loop timing errors are
measurable rather than theoretical.

**Read:**
- RM0383 — TIM2 to TIM5 chapter (32-bit vs 16-bit counter width, prescaler)
- PM0214 §4.4 — SysTick (why it's insufficient alone: 24-bit, tied to the tick rate)

---

## R7 — Preemptive Kernel
**Implement:** Task control blocks, per-task stacks, priority-based ready queue.
Context switch via **PendSV at the lowest possible priority** — SVC or a tick
handler sets the PendSV pending bit, and the switch executes only after every other
active ISR has drained. Getting this wrong (switching directly inside a high-priority
ISR) is the classic way to corrupt the stack.

The FPU is the trap on Cortex-M4F. You're building with `-mfloat-abi=hard`, so tasks
will use S0–S31. Hardware lazy stacking saves S0–S15 automatically, but **S16–S31 and
FPSCR are yours to save** in the context switch if more than one task touches float.
A flight controller's control loop and estimator both will.

**Done when:** Two tasks at different priorities provably preempt correctly. A
higher-priority task made ready inside an ISR runs immediately on ISR exit, not at
the next tick. Stack high-water marks are measurable per task.

**Exposes upward:** Concurrency. Tasks with independent deadlines.

**Read:**
- PM0214 §2.3.7 — Exception entry/exit, EXC_RETURN, and what determines MSP vs PSP
- ARMv7-M ARM §B1.5 — PendSV rationale and the tail-chaining model
- FreeRTOS `portable/GCC/ARM_CM4F/port.c` — the reference implementation of exactly
  this context switch, ~200 lines, worth reading line by line
- Joseph Yiu, *The Definitive Guide to ARM Cortex-M3 and Cortex-M4 Processors* — ch.
  on OS support (SVC, PendSV, PSP/MSP split)

---

## R8 — Synchronization Primitives
**Implement:** Counting semaphore, mutex with priority inheritance, fixed-capacity
message queue. ISR-safe "give from interrupt" variants that request a context switch
on exit rather than switching inline.

This is where `wait_transfer()` finally stops being a spin. The DMA ISR gives a
semaphore; the waiting task blocks on it and the scheduler runs something else for
the ~21 ms a bulk transfer takes. The mechanism already exists — L5 made completion
an event rather than a poll, which is precisely what makes this substitution a
drop-in.

Priority inheritance matters because of the shared bus: without it, a low-priority
logging task holding the SPI mutex can block the control loop indefinitely while a
medium-priority task hogs the CPU. That's unbounded priority inversion, and it's
what killed Mars Pathfinder.

**Done when:** No spin-waits remain anywhere in driver code. A priority inversion
scenario can be constructed, demonstrated, and shown to resolve via inheritance.

**Exposes upward:** Blocking that yields instead of burning cycles.

**Read:**
- Any RTOS text on priority inversion / priority inheritance
- The Mars Pathfinder postmortem (well documented online, and genuinely the clearest
  real-world case study of the failure mode)

---

## R9 — Sensor Pipeline
**Implement:** IMU over SPI (ICM-42688-P, BMI270, or MPU-6000 depending on what the
build uses), read via DMA into double buffers. Critically: the loop is driven by the
IMU's **data-ready interrupt on EXTI**, not by a timer. Phase-locking to actual
sample availability instead of free-running against it eliminates the beat frequency
between your sampling and the sensor's internal ODR — that beat shows up as
low-frequency noise in the attitude estimate and is genuinely hard to diagnose later.

Then barometer, magnetometer, GPS (UART, variable-rate, NMEA or UBX).

**Done when:** Gyro sampled at 4–8 kHz with jitter you've measured and can quote.
Sensor data is timestamped at capture, not at processing.

**Exposes upward:** Timestamped, deterministic sensor stream.

**Read:**
- Datasheet for the specific IMU — register map, ODR configuration, FIFO behavior,
  data-ready pin semantics
- RM0383 §8.3 / EXTI chapter — external interrupt line configuration

---

## R10 — Control & Actuation
**Implement:** Attitude estimation (complementary filter first — it's ~30 lines and
you can reason about it; Mahony or EKF later if warranted). Cascaded PID: rate loop
inner at 4–8 kHz, attitude loop outer at ~500 Hz. Motor output via DShot — DMA feeding
timer CCR values to generate the bit timing, since DShot encodes bits as pulse widths
and bit-banging it from the CPU wastes the determinism you just built.

**Done when:** Bench test with props off: the airframe resists manual rotation
correctly on all three axes and the response is stable across the throttle range.

**Exposes upward:** Stabilized flight.

**Read:**
- Betaflight source — `src/main/flight/pid.c` and `src/main/drivers/dshot*`, the
  de-facto reference for both
- PX4 or ArduPilot for a more architecturally formal treatment of the same problem

---

## R11 — Safety & Failsafe
**Implement:** Independent watchdog (IWDG) petted only from the control loop, so a
stalled loop forces a reset rather than a fly-away. Arming state machine with
explicit preconditions. Failsafe on RC signal loss, on link loss, on battery
threshold. Bounded worst-case execution time for every task, with stack usage
analysis to prove no overflow.

This layer has no counterpart in the image OS, and it's the one that most changes how
you write everything below it. No unbounded loops, no VLAs, no dynamic allocation
after init — every buffer sized at compile time and every path's worst case known.

**Done when:** Every identified failure mode has a defined, tested response. Killing
the control task in the debugger triggers a watchdog reset within the expected window.

**Exposes upward:** The property that the aircraft fails safe rather than
unpredictably.

**Read:**
- RM0383 — IWDG chapter (note it runs off the independent LSI clock, so it survives
  a main-clock failure — that's the entire point of choosing it over WWDG)

---

## R12 — Telemetry & Blackbox
**Implement:** Blackbox logging to flash or SD from the **lowest-priority** task,
fed by a ring buffer written from the control loop. The control loop must never
block on storage. Live telemetry over UART (MSP, MAVLink, or CRSF depending on the
radio).

**Done when:** A full flight logs without a single dropped sample, and the control
loop's measured jitter is unchanged with logging enabled versus disabled. That second
condition is the real test.

**Exposes upward:** Post-flight tuning and diagnosis.

**Read:**
- Betaflight blackbox format documentation
- MSP or MAVLink protocol specification, whichever the ground station speaks

---

## What actually transfers between the two branches

**Directly reusable:** L1–L5, with the three L4/L5 retrofits above. Boot, clock tree,
UART, vector table, SPI+DMA transport.

**Conceptually reusable, different implementation:** the driver *pattern* from L6 —
a class holding a bus reference, hardware detail behind a narrow interface. The
ST7735R and W5500 drivers themselves don't transfer, but the shape does.

**Flows backward into the image OS:** R7's scheduler and R8's blocking primitives are
strictly better than what L7/L8 sketch. If the RTOS branch gets there first, the
image OS should adopt them rather than build a second, weaker version — L8's
"optional: minimal cooperative scheduler" is exactly the thing R7 supersedes.

**Doesn't transfer either direction:** R9–R12 (drone-specific) and L9–L11
(image-protocol-specific).
