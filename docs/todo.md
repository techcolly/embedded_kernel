# TODO

Deferred work marked in code comments. Update as items are completed.

## Display (L6)

- **display.cpp:78** — verify MADCTL `0x08` (BGR) is correct for the panel; change if colors are wrong
- **display.cpp:89** — optimize `fill()`: 32KB static buffer permanently occupies BSS, sends in one shot. Use chunked sends or a smaller repeating buffer
- **display.cpp:116** — optimize `displayTestPattern()`: 7 SPI transactions per pixel. Build one buffer and stream it like `fill()` does

## SPI (L5)

- **spi.cpp:145** — add detailed `Status` values instead of a single generic `FAIL`

## UART (L3)

- **uart.cpp:86, 105** — replace spin-count timeouts (`37500000UL`) with a real timer
- **uart.cpp:92** — add error checking on the `USART_DR` write

## Boot (L1/L2)

- **startup.c:261** — make `clock_init()` return a real failure value instead of always 0

## Constants

- **constants.h** — make `RGB565` a real projection: scale R and B by 31/255, G by 63/255 instead of truncating low bits
