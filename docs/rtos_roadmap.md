# The RTOS Route

This document is two things at once: an explanation of how a real-time operating
system actually works on this chip, and a concrete build plan. Read it start to
finish the first time. After that, Parts 3 and 4 are the checklist.

RTOS stands for **real-time operating system**. "Real-time" doesn't mean fast — it
means *predictable*. A system is real-time if you can state a worst-case deadline and
prove it's met. A desktop OS is fast but not real-time; it may pause for 50 ms to
garbage-collect and nobody dies.

---

# Part 0 — Why this instead of memory protection

The original plan had L7 as memory protection: configure the MPU (**memory protection
unit** — hardware that blocks reads/writes to addresses a program shouldn't touch),
drop the CPU into unprivileged mode, and route driver access through system calls.

The argument against doing that now is sound. Memory protection draws a boundary
between two parts of a program. Right now there's only one part. `main()` calls
`display.init()`, then `fill()`, then `displayTestPattern()`, then spins forever. A
line between "kernel" and "application" would be arbitrary, because there's one
thread of control doing one thing at a time.

Worse, MPU regions are configured **per task** in any real design — you reprogram them
on every task switch so task A physically cannot touch task B's memory. Setting them
up against a single-threaded program means setting them up again later.

There's also a concrete cost. Every driver you have writes hardware registers
directly: `Uart::write_byte` writes `USART_DR`, `Display::write_command` writes
`GPIOB_BSRR`. Unprivileged mode blocks all of that. You'd break every driver to
demonstrate a boundary protecting nothing.

So: build the scheduler first, get real tasks, then protect them. Part 5 covers the
MPU work once it means something.

---

# Part 1 — What an RTOS actually is

This part is conceptual. No build steps. If you already know it, skim to Part 2 —
but the vocabulary here is used throughout the rest.

## 1.1 A task is a function plus a stack

That's the whole idea. Strip away the terminology and a task is:

- A function that never returns (it loops forever)
- A block of memory used as its stack
- A small record noting where its stack pointer currently is

You have three of these in mind for this project: one driving the display, one
handling the W5500 network chip, one running protocol logic. Each is an ordinary C++
function. What makes them *tasks* is that each gets its own stack, and something
switches between them.

The word "task" and the word "thread" mean the same thing here. Embedded people say
task, desktop people say thread. There's no technical distinction worth caring about.

## 1.2 What "context" means

You built a CPU in Logisim, so this will land quickly: **context is the register
file**.

When the processor is executing task A, the registers hold task A's values — `r0`
through `r12` hold its variables, `SP` (stack pointer) points into its stack, `PC`
(program counter) points at its next instruction, and the status register holds its
condition flags. That collection *is* task A, as far as the hardware is concerned.
There is nothing else.

Switching to task B means: copy all of that somewhere safe, load task B's saved copy
into the same registers, and continue. The CPU has no idea anything happened. It just
found different values in its registers than it left there.

This is why a context switch is written in assembly. There's no way to express "save
every register" in C, because C is already using the registers to do things.

## 1.3 The hardware does half the work for you

Here's the part that makes this tractable on ARM.

When any interrupt fires on a Cortex-M4, the hardware automatically pushes eight
values onto the current stack before running your handler:

```
r0, r1, r2, r3, r12, LR, PC, xPSR
```

You've already relied on this without thinking about it. Your `SysTick_Handler` is a
plain C function that increments `millis_count`, and it doesn't corrupt whatever
`main()` was doing — because the hardware saved the registers the handler was about
to clobber.

Why those eight specifically? Because of **AAPCS**, the ARM Architecture Procedure
Call Standard — the rules compilers follow about which registers a function may
freely overwrite. It splits registers into two groups:

- `r0-r3, r12` — **caller-saved**. A function may destroy these. If the caller cared,
  the caller saved them first.
- `r4-r11` — **callee-saved**. A function that wants to use these must save and
  restore them itself.

An interrupt can fire between any two instructions, so the hardware saves the
caller-saved group. That's exactly enough for your handler to be a normal C function
that the compiler treats like any other.

For a *context switch*, that's not enough. You're not returning to the same task, so
`r4-r11` — which the interrupted task is relying on being preserved — must be saved
too. That's the manual half.

> **Read:** PM0214 §2.3.7, "Exception entry and return." The stack frame diagram there
> is worth staring at until it's obvious.
> https://www.st.com/resource/en/programming_manual/pm0214-stm32-cortexm4-mcus-and-mpus-programming-manual-stmicroelectronics.pdf

## 1.4 Two stack pointers

The Cortex-M4 has two stack pointers, and only one is visible at a time.

- **MSP** — Main Stack Pointer. Used by interrupt handlers, always.
- **PSP** — Process Stack Pointer. Optionally used by normal (non-interrupt) code.

Right now your firmware only uses MSP. Everything — `main()`, your drivers, your
interrupt handlers — shares one stack that starts at `_estack` (top of RAM) and grows
downward.

Under an RTOS you split them: handlers keep using MSP, and each task gets its own
stack that PSP points into. Two reasons:

**Isolation.** If a task overflows its stack, it corrupts its own memory. The
interrupt handlers still have a clean stack to run on, including the fault handler
that's supposed to tell you what went wrong. Sharing one stack means a stack overflow
can destroy the machinery meant to diagnose it.

**Simplicity.** Switching tasks becomes "change the value in PSP." The handler's own
stack is untouched, so the switching code has somewhere stable to stand.

Which one is active is controlled by bit 1 (`SPSEL`) of the `CONTROL` register. Bit 0
(`nPRIV`) is the privilege bit from the deferred MPU work — you'll leave that alone
for now.

> **Read:** PM0214 §2.1.3, "Core registers" — the `CONTROL` register subsection.

## 1.5 How a switch actually happens, step by step

Concretely, with task A running and task B about to run:

1. Something decides a switch is needed — usually the SysTick timer interrupt firing
   and the scheduler noticing task B has higher priority than A.
2. That code doesn't switch. It sets a bit requesting the **PendSV** exception
   (Pendable Service Call — an interrupt that exists purely to be triggered by
   software).
3. PendSV runs. On entry, hardware has already pushed A's `r0-r3, r12, LR, PC, xPSR`
   onto A's stack, and PSP points just below them.
4. The handler reads PSP, pushes `r4-r11` onto A's stack too, and stores the resulting
   PSP value into task A's record.
5. It loads task B's saved PSP value, pops `r4-r11` from B's stack, and writes the
   remaining value into PSP.
6. It returns. Hardware pops `r0-r3, r12, LR, PC, xPSR` off B's stack.
7. The CPU is now executing task B, exactly where B left off.

Steps 4 and 5 are the entire context switch. Everything else in an RTOS exists to
decide *when* to do this and *which* task to pick.

## 1.6 Why PendSV, and why at the lowest priority

Step 2 above looks like a pointless detour. Why not switch immediately?

Because interrupts nest. A DMA completion interrupt can fire while the SysTick handler
is still running. If SysTick switched stacks directly, the DMA handler would return
onto a stack that has nothing to do with the code it interrupted, and both tasks would
be corrupted.

PendSV solves this. You set it to the **lowest** priority in the system, so it can
never preempt anything. When another handler pends it, it waits. Only when every other
interrupt has finished does PendSV finally run — at which point nothing else is
mid-flight and swapping stacks is safe.

The Cortex-M makes this cheap through **tail-chaining**: when one exception finishes
and another is already pending, the hardware skips the unstack-then-restack and goes
straight into the next handler. So pending PendSV from SysTick doesn't cost a full
extra exception entry.

This is the standard pattern on every ARM RTOS. There isn't a good alternative.

> **Read:** ARMv7-M Architecture Reference Manual §B1.5, particularly the sections on
> exception prioritization and tail-chaining. More precise than PM0214 where they
> overlap. https://developer.arm.com/documentation/ddi0403/latest/

## 1.7 What the scheduler is

Less than you'd expect. The scheduler is a function that answers one question: *of all
the tasks that could run right now, which has the highest priority?*

Each task is in one of a few states:

- **Running** — currently executing. Exactly one, on a single-core chip.
- **Ready** — could run, waiting for the CPU.
- **Blocked** — waiting for something (a timer, a semaphore, data to arrive). Cannot
  be chosen.

The scheduler picks the highest-priority Ready task and switches to it. That's all.
The sophistication is in making that lookup fast and constant-time, which Part 4
covers.

## 1.8 What "blocking" means, and why it's the whole point

This is the concept that changes your existing code most, so it's worth being precise.

Right now, [display.cpp](../firmware/src/display.cpp) waits like this:

```cpp
void Display::wait_transfer() noexcept {
    while (!spi1_tx_done);
    spi1_tx_done = false;
}
```

That's a **spin-wait**. The CPU sits in a loop reading a variable, doing nothing,
until an interrupt sets it. For `fill()` — which pushes 32 KB through DMA at the
display's ÷8 prescaler, roughly 21 milliseconds — that's 21 ms of a 100 MHz processor
executing a load instruction over and over.

Under an RTOS the same wait becomes:

```cpp
sem_take(&spi1_done, TIMEOUT_MS);
```

The difference isn't syntax. `sem_take` moves the calling task out of the Ready
state entirely, so the scheduler stops considering it. Some other task runs during
those 21 ms. When the DMA interrupt fires, it marks the task Ready again, and the
scheduler picks it back up.

Nothing spins. The CPU is always doing useful work or asleep.

Here's the part worth appreciating: **you already built the mechanism for this.** Back
in L5 you made `spi__transfer()` return immediately and signal completion from an
interrupt, rather than blocking inline. At the time that looked like extra complexity
for no benefit, since every caller just spun on the flag afterward anyway. This is
where it pays off. Because completion is already an *event* rather than something you
poll, swapping the spin for a semaphore is a small change instead of a redesign.

---

# Part 2 — Decisions to make once

These are cheap now and expensive later. The drone flight controller reuses this
kernel, so "later" means "in the middle of a different project."

## 2.1 Three directories, one rule

```
kernel/port/     CPU-specific: context switch, critical sections, tick setup
kernel/core/     portable: scheduler, task records, semaphores, queues
app/             this project's tasks
```

**The rule: `kernel/core/` contains no STM32 register writes and no assembly.**

If that holds, the drone project copies `kernel/` unchanged and writes its own `app/`.
If it doesn't — if a scheduler function pokes `GPIOB_BSRR` for a debug pin, say — then
"reuse" turns into "untangle."

This is the single highest-value decision in the document. It costs nothing to follow
from the start and is tedious to fix afterward.

## 2.2 No dynamic allocation

No `malloc`, no `new`, ever. Tasks are declared with static stack arrays and static
task records:

```cpp
static uint8_t display_stack[1024];
static Task    display_task;
```

Two reasons. First, `malloc` has no bounded worst-case time — it might return
immediately or walk a long free list, and you can't predict which. Real-time means
predictable, so that's disqualifying. Second, repeated allocate/free fragments memory,
and a system running for hours can fail an allocation that succeeded a thousand times
before.

The useful side effect: total RAM usage is known at **link time**. If it fits, it
fits, permanently. No runtime surprise.

## 2.3 Fixed-priority preemptive scheduling

Every task gets a fixed priority number. The highest-priority Ready task always runs.
If a higher-priority task becomes Ready, it preempts the current one immediately.

Not round-robin (equal time slices), not earliest-deadline-first (dynamic priorities).
Fixed priority is what Betaflight, PX4, and ArduPilot all use, for one reason: you can
calculate the worst-case delay before your highest-priority task runs. With dynamic
schemes you generally can't.

## 2.4 Every blocking call takes a timeout

`sem_take(&s, 100)` — not `sem_take(&s)`.

A blocking call with no timeout is a hang with no diagnosis. With a timeout it becomes
an error you can report over UART. Retrofitting this later means touching every call
site in every driver, which is exactly the kind of change that introduces bugs in code
that was working.

## 2.5 One header owns every priority number

Both interrupt priorities and task priorities, in one file. Priority bugs are hard to
see when the numbers are scattered across drivers — you end up reading six files to
answer "can this interrupt preempt that one."

---

# Part 3 — Retrofits to L1–L5

Do these before starting Part 4. None are large; several are load-bearing.

## 3.1 Interrupt priority map

**What:** Assign an explicit priority to every interrupt you've enabled, from one
header.

Some specifics about this chip. The STM32F411 implements **4 priority bits**, and they
sit in the **high nibble** of an 8-bit field. So valid values are `0x00, 0x10, 0x20,
… 0xF0` — sixteen levels, and writing `0x01` is the same as writing `0x00`.

**Lower number means higher urgency.** Priority `0x00` preempts priority `0x10`. This
is backwards from intuition and trips everyone up at least once.

Your `Reset_Handler` already sets `AIRCR` with PRIGROUP = 3. With 4 implemented bits
that means all 4 bits are preemption priority and none are sub-priority, which is
exactly what an RTOS wants. No change needed — worth knowing you got it right.

The map:

| Priority | What goes here | May call kernel functions? |
|---|---|---|
| `0x00` | SVCall — runs once, to start the scheduler | n/a |
| `0x10`–`0x30` | Time-critical interrupts. Empty for now; the drone's gyro data-ready goes here | **No** |
| `0x40`–`0xD0` | Normal peripherals: DMA, UART, SPI | Yes |
| `0xE0` | SysTick | Yes |
| `0xF0` | PendSV | n/a |

**Why the "may call kernel functions" column matters** — this is the most common source
of subtle RTOS corruption, so it's worth understanding rather than memorizing.

Critical sections (3.2) work by masking interrupts up to a threshold. Interrupts
*above* that threshold are never masked, which is the point — a gyro interrupt should
never be delayed by kernel bookkeeping. But it also means those interrupts can fire
while the kernel is halfway through updating its own data structures. If such an
interrupt then called `sem_give()`, it would corrupt a list mid-modification.

So the rule is: interrupts above the threshold get low latency, and in exchange they
may not touch the kernel at all. They can write to a plain variable or a lock-free
buffer, and a lower-priority interrupt or task picks it up.

FreeRTOS calls this threshold `configMAX_SYSCALL_INTERRUPT_PRIORITY` and their
documentation on it is the clearest explanation available:
https://www.freertos.org/RTOS-Cortex-M3-M4.html

Priorities for SysTick and PendSV live in the System Handler Priority Registers
(`SHPR2`/`SHPR3` at `0xE000ED1C`/`0xE000ED20`), not in `NVIC_IPR` — those are for
peripheral interrupts only. Easy to miss.

> **Read:** PM0214 §4.3 for NVIC priority registers, §4.4 for the system handler
> priority registers.

## 3.2 Critical sections using BASEPRI

**What:** A pair of functions that temporarily block interrupts while the kernel
modifies shared data, then restore the previous state.

The naive version is `cpsid i` — the instruction that disables all interrupts by
setting `PRIMASK`. Don't use it. It blocks *everything*, including the time-critical
interrupts you just promised wouldn't be delayed.

Use `BASEPRI` instead. Writing a priority value into `BASEPRI` masks all interrupts at
that priority **or lower urgency**, while anything more urgent still gets through. Set
it to your threshold (`0x40` in the table above) and the gyro interrupt stays live
through every critical section.

One implementation detail that matters: the functions must nest. Kernel functions call
each other, and if `critical_exit()` unconditionally re-enables interrupts, an inner
call will re-enable them inside an outer critical section. The fix is to have
`critical_enter()` return the old `BASEPRI` value and `critical_exit(old)` restore it.

> **Read:** PM0214 §2.1.3 — the `PRIMASK`, `FAULTMASK`, and `BASEPRI` subsections.

## 3.3 Stack layout and a guard region

**What:** Rework [linker.ld](../firmware/linker.ld).

Currently `_estack` is `ORIGIN(RAM) + LENGTH(RAM)` — top of RAM — and the stack grows
down toward `.bss` with nothing in between. If it overflows, it silently overwrites
your globals. There's no error, just wrong values appearing somewhere unrelated.

Add:

- A defined-size stack region for MSP (the interrupt handler stack)
- A gap below it, at least 32 bytes, that nothing is allowed to use
- RAM arranged in power-of-two, size-aligned blocks

**Why the alignment when you're deferring the MPU:** the MPU requires each protected
region's base address to be aligned to that region's own size — a 4 KB region must
start on a 4 KB boundary. That constraint shapes the memory layout. Doing it now costs
one careful afternoon; doing it after tasks exist means moving everything.

Task stacks are static arrays, so they land in `.bss` automatically. Their guard is a
different mechanism — see 4.7.

> **Read:** ARMv7-M ARM §B3.5 on MPU region alignment. Read it before writing the
> linker script, not after.

## 3.4 UART stops blocking

**What:** [uart.cpp](../firmware/src/uart.cpp) currently spins on the `TXE` flag with
a fake timeout (`volatile uint32_t timer = 37500000UL` counting down). Convert to
interrupt-driven with a ring buffer, or DMA, signalling completion with a semaphore.

**Why:** at 115200 baud each byte takes about 87 microseconds. With one thread of
control, spinning through that is invisible. With tasks, it's 87 µs per byte stolen
from whatever should have been running — and a 40-character debug line is 3.5
milliseconds.

There's a second problem. `uart_write_isr()` is called from
`DMA2_Stream3_IRQHandler` in [startup.c](../firmware/src/startup.c). Once UART writes
can block, calling one from an interrupt handler deadlocks — the handler waits for a
task to run, but the task can't run until the handler returns. The fix is a lock-free
ring buffer that interrupts push into and a low-priority task drains.

## 3.5 SPI needs per-device descriptors

This is the largest rewrite risk in the existing code, so it's worth doing carefully.

**What:** [spi.cpp](../firmware/src/spi.cpp) stores `cs_gpio_base_` and `cs_pin_` as
members of `Spi`, and fixes the clock prescaler in the constructor. That hardcodes
**one device per SPI port**, permanently.

Restructure so the device is named per transfer:

```cpp
struct SpiDevice {
    uint32_t cs_gpio_base;
    uint32_t cs_pin;
    uint32_t prescaler;   // clock divider — reprogrammed per transaction
    uint32_t mode;        // CPOL/CPHA
};

void spi__transfer(const SpiDevice& dev, const uint8_t* tx, uint8_t* rx, uint16_t len);
```

**Why:** the drone puts an inertial measurement unit, a barometer, a magnetometer, and
often a flash chip on a single SPI bus. Each has a different maximum clock speed —
the IMU might handle 24 MHz while the barometer tops out at 10 MHz. The current design
cannot express that at all; the prescaler is decided once, at construction, for the
whole bus.

While you're in there: the `w5500_full_duplex` boolean parameter is a device-specific
flag sitting in the generic transport layer. Full duplex is a property of the
*transfer*, not of the W5500. Make it implicit — `rx != nullptr` means full duplex —
so the IMU can use the same path.

## 3.6 Bus arbitration

**What:** A mutex per SPI bus, held for the duration of a transaction. Completion
signalled by a per-bus semaphore rather than the current global `spi1_tx_done` and
`spi2_rx_done` booleans.

**Why:** two tasks issuing transfers on the same bus will interleave their chip-select
assertions and corrupt both transactions. The mutex serializes them.

The mutex must support **priority inheritance** — see 4.5 for what that is and why it
matters here specifically.

## 3.7 A microsecond timer

**What:** TIM2 or TIM5 free-running at 1 MHz, giving you `micros()` alongside the
existing `millis()`.

Use TIM2 or TIM5 specifically because they're the **32-bit** timers on this chip.
TIM3 and TIM4 are 16-bit, which at 1 MHz wraps every 65 milliseconds — useless for
measuring anything.

**Why:** the 1 ms SysTick can't measure a context switch (a few microseconds), task
runtime, or timing jitter. You need this for the diagnostics in 4.7, and the drone
needs microsecond timestamps on every sensor sample.

---

# Part 4 — Building the kernel

Seven stages, each independently testable. That matters more here than anywhere else
in this project: scheduler bugs corrupt stacks silently, and the crash surfaces
somewhere unrelated, several function calls later. Testing each stage before moving on
is the difference between a week and a month.

## 4.1 — Split the stacks

**Build:** Set PSP to a stack top, set the `SPSEL` bit in `CONTROL`, then execute an
`ISB` instruction. `main()` keeps running, now using PSP instead of MSP.

`ISB` is an **Instruction Synchronization Barrier**. The processor pipelines
instructions, so after changing something as fundamental as which stack pointer is
active, you must flush the pipeline or the next few instructions may execute with
stale state. This is architecturally required, not a superstition.

**Test:** In the debugger, confirm SP equals PSP inside `main()` and MSP inside
`SysTick_Handler`. The program should otherwise behave identically.

**Why first:** everything else depends on it, and it's a small change with an
unambiguous pass/fail.

## 4.2 — The context switch

**Build:** `PendSV_Handler`, in assembly. The sequence from 1.5:

1. Read PSP into a register
2. Push `r4-r11` onto the task's stack
3. If the task used the floating-point unit, push `s16-s31` too
4. Store the resulting stack pointer into the outgoing task's record
5. Load the incoming task's saved stack pointer
6. Pop `r4-r11` (and floating-point registers if applicable)
7. Write to PSP and return with an `EXC_RETURN` value selecting PSP and Thread mode

`EXC_RETURN` is the special value in `LR` on exception entry. It isn't a return
address — it's a code telling the hardware how to unwind: which stack to pop from, and
whether floating-point registers are in the frame.

**The floating-point trap.** You build with `-mfloat-abi=hard`, so the FPU is in use.
The hardware saves `s0-s15` and `FPSCR` automatically (this is called *lazy stacking*),
but `s16-s31` are callee-saved and are your problem — the same split as `r4-r11`.

Bit 4 of `EXC_RETURN` tells you whether this task has floating-point state: clear
means the frame includes it. Test the bit rather than always saving, or you waste 64
bytes of stack and roughly 30 cycles on every switch of every task that never touches
a float.

Getting this wrong produces silent numeric corruption in whichever task got preempted.
No fault, no crash — just wrong numbers. It's among the worst bugs in this document to
diagnose, which is why it's worth handling correctly the first time rather than
deferring it. The drone's attitude estimator and control loop both use floats
constantly.

**Test:** Two tasks alternating via an explicit `yield()` that pends PendSV. Each keeps
a local counter and prints it. If both counters advance independently and neither
task's locals get corrupted, the switch works.

> **Read this one line by line:** FreeRTOS's `portable/GCC/ARM_CM4F/port.c` and
> `portmacro.h`. That's the reference implementation of 4.1 and 4.2 for exactly this
> processor with exactly this floating-point configuration — same core, same compiler,
> same ABI. About 200 lines of substance.
> https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/portable/GCC/ARM_CM4F/port.c

> **Also good:** Miro Samek's "Modern Embedded Systems Programming" video course builds
> a preemptive kernel for Cortex-M from nothing, one mechanism at a time. Lessons 22
> through 27 cover the context switch and scheduler. It's the closest thing to this
> roadmap in video form. https://www.youtube.com/@StateMachineCOM

## 4.3 — The scheduler

**Build:**

- A task record (**TCB**, Task Control Block) holding: saved stack pointer, priority,
  state, and links for whatever list the task is currently in.
- A ready queue: one linked list per priority level, plus a 32-bit word where bit *n*
  is set if priority level *n* has any Ready tasks.
- The SysTick handler pends PendSV.
- An **idle task** at the lowest priority.

**Why the bitmap:** finding the highest-priority ready task by scanning an array is
O(n) and takes variable time depending on where the answer is. With a bitmap, the
answer is a single `CLZ` instruction — **Count Leading Zeros**, which the Cortex-M4
has in hardware. Constant time, a couple of cycles, regardless of task count. That
constant-time property is what makes worst-case latency calculable, which is the
entire point of a real-time system.

**Why the idle task is mandatory:** the scheduler must always have something to
choose. If every task blocks simultaneously and there's no idle task, the scheduler
has no valid answer and the system faults. The idle task also gives you a natural home
for `WFI` (Wait For Interrupt — puts the core to sleep until something happens) and
for the stack checking in 4.7.

**Test:** Three tasks at different priorities. A high-priority task made Ready inside
an interrupt handler should start executing the moment the handler returns — not at
the next timer tick. That distinction is the difference between preemptive and
cooperative, and it's the thing to verify.

## 4.4 — Delays and timeouts

**Build:** `task_delay(ms)` that moves the caller to a delayed list with a wake-up
tick, and a tick handler that wakes tasks whose time has come. Timeout support
threaded through every blocking call.

**Why a sorted list rather than scanning:** the tick handler runs 1000 times a second,
and every task pays for its cost. Scanning all task records each tick is O(n) per
tick. A list sorted by wake time makes the common case — nothing to wake — a single
comparison against the head.

**Note on the existing `delay()`:** the one in [main.cpp](../firmware/src/main.cpp)
spins on `millis_count`. Keep it for startup code that runs before the scheduler
begins, and use `task_delay()` everywhere afterward. Mixing them up means a task
silently burning its full time slice.

## 4.5 — Semaphores, mutexes, queues

**Build:** A counting semaphore, a mutex with priority inheritance, and a
fixed-capacity queue. Each takes a timeout. Each needs an interrupt-safe variant.

**What these are, briefly:**

- A **semaphore** is a counter with a waiting list. `give` increments it (or wakes a
  waiter); `take` decrements it (or blocks). Used for signalling: the DMA interrupt
  gives, the waiting task takes.
- A **mutex** (mutual exclusion) is a semaphore that also tracks who owns it, so only
  the owner can release it. Used for protecting a shared resource like an SPI bus.
- A **queue** passes data between tasks with a fixed-size buffer, blocking the sender
  when full and the receiver when empty.

**Why the interrupt-safe variants are separate functions:** an interrupt handler can't
block. If a handler tried to `take` a semaphore that's unavailable, there's no task to
suspend — you'd be suspending an interrupt, which isn't a thing. So the ISR versions
never block, and they return a flag indicating whether the give woke a task more
important than the one that was interrupted. If so, the handler pends PendSV on the
way out, and the switch happens right after it returns.

**Why priority inheritance matters** — this is worth understanding properly, because
the failure is invisible until it isn't:

A low-priority logging task takes the SPI mutex and starts a transfer. A high-priority
control task tries to take the same mutex and blocks. So far, correct and expected —
the high-priority task waits briefly.

Now a *medium*-priority task becomes ready. It wants nothing from the mutex. But it
outranks the logging task, so it preempts it. The logging task stops running, so it
never releases the mutex, so the high-priority task stays blocked — by a
medium-priority task that has nothing to do with the resource. There's no bound on how
long this lasts.

That's **unbounded priority inversion**. Priority inheritance fixes it: when a
high-priority task blocks on a mutex, the owner temporarily inherits that priority, so
it can't be preempted by anything in between. It finishes, releases, and drops back.

This exact bug caused the Mars Pathfinder rover to repeatedly reset on the surface of
Mars in 1997. Glenn Reeves' internal JPL account of diagnosing it remotely is widely
mirrored and worth reading before you implement this — it's the clearest description
of the failure mode in existence. Search "What really happened on Mars Rover
Pathfinder."

**Test:** Construct the three-task inversion scenario deliberately and show that it
resolves with inheritance enabled and hangs without it.

## 4.6 — Convert the drivers

**Build:** Replace every spin-wait with a blocking call.

- `Display::wait_transfer()` → `sem_take(&spi1_done, timeout)`
- `W5500::wait_transfer()` → `sem_take(&spi2_done, timeout)`
- SPI transactions take the bus mutex first
- UART writes block on completion

**Why this is the payoff:** `fill()` moves 32 KB, about 21 ms of DMA. Today that's 21
ms of the CPU reading a `volatile bool` in a loop. After this, it's 21 ms available to
the network task. Same for every display command, every W5500 register read.

**Test:** Run the display task and the W5500 task simultaneously. Both should make
progress, neither should interfere, and total time should be noticeably less than
running them one after the other.

## 4.7 — Diagnostics

**Build:**

- **Stack high-water marks.** Fill each task stack with a known byte pattern at
  creation. Periodically scan from the low end to find the deepest point that's been
  disturbed. That tells you actual peak usage.
- **CPU usage per task**, sampled with the microsecond timer from 3.7.
- **Context switch count and worst-case switch time.**

**Why this isn't optional:** stack overflow under an RTOS doesn't announce itself. A
task that overruns its stack writes into whatever is adjacent in `.bss` — often another
task's stack or task record. The symptom appears somewhere else entirely, at some later
time, in code that's perfectly correct. Watermarking is the cheap way to know your
stack sizes are right before that happens.

The drone needs this for worst-case execution time analysis, where it's a safety
requirement rather than a nicety.

> **Read:** the Memfault Interrupt blog has the best practical writing on Cortex-M
> fault debugging and stack overflow detection. https://interrupt.memfault.com/blog/
> — search for "HardFault" and "stack overflow."

---

# Part 5 — Memory protection, revisited

With tasks in place, the deferred L7 work becomes both meaningful and straightforward:

- Reprogram MPU regions on every context switch, so each task's stack is inaccessible
  to every other task. A bug in the network task physically cannot corrupt the display
  task.
- Put a no-access region below each task stack. Overflow becomes an immediate fault
  naming the exact address, instead of silent corruption found days later.
- Optionally run tasks unprivileged, with driver access through `SVC` (Supervisor
  Call) instructions.

One thing to do early regardless of the rest: enable the MemManage fault by setting
bit 16 of `SHCSR` (System Handler Control and State Register, at `0xE000ED24`).
Without it, MPU violations escalate to a generic HardFault and you lose the
`MMFSR` status bits and the `MMFAR` fault address register. That's the difference
between "the MPU blocked a write to 0x2001F000" and "something went wrong somewhere."

> **Read:** PM0214 §4.5 for MPU registers; ARMv7-M ARM §B3.5 for the authoritative
> description of region rules and permission encodings.
