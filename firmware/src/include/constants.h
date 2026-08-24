#pragma once
#include <stdint.h>

// ── RCC (Reset and Clock Control) ─────────────────────────────────────────
// RM0383 §6.3

#define RCC_BASE            0x40023800UL

#define RCC_CR              (*(volatile uint32_t*)(RCC_BASE + 0x00UL))
#define RCC_PLLCFGR         (*(volatile uint32_t*)(RCC_BASE + 0x04UL))
#define RCC_CFGR            (*(volatile uint32_t*)(RCC_BASE + 0x08UL))
#define RCC_AHB1ENR         (*(volatile uint32_t*)(RCC_BASE + 0x30UL))
#define RCC_APB1ENR         (*(volatile uint32_t*)(RCC_BASE + 0x40UL))
#define RCC_APB2ENR         (*(volatile uint32_t*)(RCC_BASE + 0x44UL))

// RCC_CR bits (RM0383 §6.3.1)
#define RCC_CR_HSION        (1UL << 0)   // HSI oscillator enable
#define RCC_CR_HSIRDY       (1UL << 1)   // HSI oscillator ready (read-only)
#define RCC_CR_PLLON        (1UL << 24)  // PLL enable
#define RCC_CR_PLLRDY       (1UL << 25)  // PLL locked (read-only)

// RCC_PLLCFGR bits (RM0383 §6.3.2)
// PLLM:   bits [5:0]   — input divider
// PLLN:   bits [14:6]  — multiplier
// PLLP:   bits [17:16] — output divider: 0b00=÷2, 0b01=÷4, 0b10=÷6, 0b11=÷8
// PLLSRC: bit  [22]    — clock source: 0=HSI, 1=HSE
#define RCC_PLLCFGR_PLLSRC_HSI  (0UL << 22)
#define RCC_PLLCFGR_PLLSRC_HSE  (1UL << 22)

// RCC_CFGR bits (RM0383 §6.3.3)
#define RCC_CFGR_SW_HSI     0x00000000UL  // SYSCLK = HSI
#define RCC_CFGR_SW_HSE     0x00000001UL  // SYSCLK = HSE
#define RCC_CFGR_SW_PLL     0x00000002UL  // SYSCLK = PLL
#define RCC_CFGR_SW_MASK    0x00000003UL

#define RCC_CFGR_SWS_PLL    (0x2UL << 2)  // SWS bits [3:2] — PLL selected
#define RCC_CFGR_SWS_MASK   (0x3UL << 2)

// AHB prescaler:  bits [7:4]   — 0xxx=÷1, 1000=÷2, 1001=÷4 ...
// APB1 prescaler: bits [12:10] — 0xx=÷1, 100=÷2, 101=÷4, 110=÷8, 111=÷16
// APB2 prescaler: bits [15:13] — same encoding as APB1
#define RCC_CFGR_PPRE1_DIV2 (0x4UL << 10)
#define RCC_CFGR_PPRE1_MASK (0x7UL << 10)
#define RCC_CFGR_PPRE2_MASK (0x7UL << 13)

// RCC_AHB1ENR bits (RM0383 §6.3.10)
#define RCC_AHB1ENR_GPIOAEN (1UL << 0)
#define RCC_AHB1ENR_GPIOBEN (1UL << 1)
#define RCC_AHB1ENR_GPIOCEN (1UL << 2)
#define RCC_AHB1ENR_DMA1EN  (1UL << 21)
#define RCC_AHB1ENR_DMA2EN  (1UL << 22)

// RCC_APB1ENR bits (RM0383 §6.3.12)
#define RCC_APB1ENR_USART2EN (1UL << 17)
#define RCC_APB1ENR_SPI2EN   (1UL << 14)
#define RCC_APB1ENR_SPI3EN   (1UL << 15)

// RCC_APB2ENR bits (RM0383 §6.3.13)
#define RCC_APB2ENR_USART1EN (1UL << 4)
#define RCC_APB2ENR_SPI1EN   (1UL << 12)

// ── Flash Interface ────────────────────────────────────────────────────────
// RM0383 §3.7

#define FLASH_R_BASE        0x40023C00UL

#define FLASH_ACR           (*(volatile uint32_t*)(FLASH_R_BASE + 0x00UL))

// FLASH_ACR bits (RM0383 §3.7.1)
// At 100 MHz / 3.3V: 3 wait states required (RM0383 §3.4.1 Table 6)
#define FLASH_ACR_LATENCY_MASK  0x0000000FUL
#define FLASH_ACR_LATENCY_0WS   0x00000000UL
#define FLASH_ACR_LATENCY_1WS   0x00000001UL
#define FLASH_ACR_LATENCY_2WS   0x00000002UL
#define FLASH_ACR_LATENCY_3WS   0x00000003UL
#define FLASH_ACR_PRFTEN        (1UL << 8)   // prefetch enable
#define FLASH_ACR_ICEN          (1UL << 9)   // instruction cache enable
#define FLASH_ACR_DCEN          (1UL << 10)  // data cache enable

// ── GPIO (shared constants) ────────────────────────────────────────────────
// RM0383 §8.4

// MODER field values — 2 bits per pin (RM0383 §8.4.1)
#define GPIO_MODER_INPUT    0x0UL
#define GPIO_MODER_OUTPUT   0x1UL
#define GPIO_MODER_AF       0x2UL
#define GPIO_MODER_ANALOG   0x3UL

// Alternate function numbers — 4 bits per pin in AFR (DS10314 Table 9)
#define GPIO_AF5            5UL   // SPI1/2/3
#define GPIO_AF7            7UL   // USART1/2 on PA2/PA3/PA9/PA10

// USART pin assignments on GPIOA (DS10314 Table 9)
#define USART2_TX_PIN       2U    // PA2  AF7
#define USART2_RX_PIN       3U    // PA3  AF7
#define USART1_TX_PIN       9U    // PA9  AF7
#define USART1_RX_PIN       10U   // PA10 AF7

// SPI1 pin assignments on GPIOA (DS10314 Table 9)
#define SPI1_SCK_PIN        5U    // PA5  AF5
#define SPI1_MISO_PIN       6U    // PA6  AF5
#define SPI1_MOSI_PIN       7U    // PA7  AF5
#define SPI1_CS_PIN         4U    // PA4  plain output — CS is software-driven, not AF

// SPI2 pin assignments on GPIOB (DS10314 Table 9)
#define SPI2_SCK_PIN        13U   // PB13 AF5
#define SPI2_MISO_PIN       14U   // PB14 AF5
#define SPI2_MOSI_PIN       15U   // PB15 AF5
#define SPI2_CS_PIN         12U   // PB12 plain output — CS is software-driven, not AF

// bit position of a pin's field within each GPIO register (RM0383 §8.4)
#define GPIO_MODER_POS(pin)    ((pin) * 2)   // MODER, OSPEEDR, PUPDR — 2 bits per pin
#define GPIO_OSPEEDR_POS(pin)  ((pin) * 2)
#define GPIO_AFRL_POS(pin)     ((pin) * 4)          // AFRL covers pins 0-7
#define GPIO_AFRH_POS(pin)     (((pin) - 8) * 4)    // AFRH covers pins 8-15
#define GPIO_BSRR_SET(pin)     (1UL << (pin))       // drive pin high
#define GPIO_BSRR_RESET(pin)   (1UL << ((pin) + 16))// drive pin low

// ── GPIO A ─────────────────────────────────────────────────────────────────
// RM0383 §8.4, DS10314 Table 9

#define GPIOA_BASE          0x40020000UL

#define GPIOA_MODER         (*(volatile uint32_t*)(GPIOA_BASE + 0x00UL))
#define GPIOA_OSPEEDR       (*(volatile uint32_t*)(GPIOA_BASE + 0x08UL))

#define GPIOA_AFRL          (*(volatile uint32_t*)(GPIOA_BASE + 0x20UL))  // alternate function low (pins 0-7)
#define GPIOA_AFRH          (*(volatile uint32_t*)(GPIOA_BASE + 0x24UL))  // alternate function high (pins 8-15)
#define GPIOA_BSRR          (*(volatile uint32_t*)(GPIOA_BASE + 0x18UL))

// ── GPIO B ─────────────────────────────────────────────────────────────────
// RM0383 §8.4

#define GPIOB_BASE          0x40020400UL

#define GPIOB_MODER         (*(volatile uint32_t*)(GPIOB_BASE + 0x00UL))
#define GPIOB_OSPEEDR       (*(volatile uint32_t*)(GPIOB_BASE + 0x08UL))
#define GPIOB_AFRH          (*(volatile uint32_t*)(GPIOB_BASE + 0x24UL))  // alternate function high (pins 8-15)
#define GPIOB_BSRR          (*(volatile uint32_t*)(GPIOB_BASE + 0x18UL))

// ── GPIO C ─────────────────────────────────────────────────────────────────
// RM0383 §8.4

#define GPIOC_BASE          0x40020800UL

#define GPIOC_MODER         (*(volatile uint32_t*)(GPIOC_BASE + 0x00UL))
#define GPIOC_OTYPER        (*(volatile uint32_t*)(GPIOC_BASE + 0x04UL))
#define GPIOC_OSPEEDR       (*(volatile uint32_t*)(GPIOC_BASE + 0x08UL))
#define GPIOC_PUPDR         (*(volatile uint32_t*)(GPIOC_BASE + 0x0CUL))
#define GPIOC_BSRR          (*(volatile uint32_t*)(GPIOC_BASE + 0x18UL))

#define LED_PIN             13U

// ── USART2 ─────────────────────────────────────────────────────────────────
// RM0383 §19.6

#define USART2_BASE         0x40004400UL

#define USART2_SR           (*(volatile uint32_t*)(USART2_BASE + 0x00UL))  // status register
#define USART2_DR           (*(volatile uint32_t*)(USART2_BASE + 0x04UL))  // data register
#define USART2_BRR          (*(volatile uint32_t*)(USART2_BASE + 0x08UL))  // baud rate register
#define USART2_CR1          (*(volatile uint32_t*)(USART2_BASE + 0x0CUL))  // control register 1

// USART_SR bits (RM0383 §19.6.1)
#define USART_SR_RXNE       (1UL << 5)   // read data register not empty
#define USART_SR_TC         (1UL << 6)   // transmission complete
#define USART_SR_TXE        (1UL << 7)   // transmit data register empty

// USART_CR1 bits (RM0383 §19.6.4)
#define USART_CR1_RE        (1UL << 2)   // receiver enable
#define USART_CR1_TE        (1UL << 3)   // transmitter enable
#define USART_CR1_UE        (1UL << 13)  // USART enable
#define USART_CR1_OVER8     (1UL << 15)  // 0=16x oversampling (default), 1=8x

// USART_BRR fields (RM0383 §19.6.3) — 16x oversampling: USARTDIV = fCK / (16 * baud)
// mantissa = integer part, fraction = fractional part * 16
#define USART_BRR_MANTISSA(m)  ((uint32_t)(m) << 4)
#define USART_BRR_FRACTION(f)  ((uint32_t)(f) & 0xFUL)

// Generic base-relative USART register access — use these inside the Uart class
// where base_ is a runtime value (RM0383 §19.6)
#define USART_SR(base)      (*(volatile uint32_t*)((base) + 0x00UL))
#define USART_DR(base)      (*(volatile uint32_t*)((base) + 0x04UL))
#define USART_BRR(base)     (*(volatile uint32_t*)((base) + 0x08UL))
#define USART_CR1(base)     (*(volatile uint32_t*)((base) + 0x0CUL))

// -- USART1

#define USART1_BASE         0x40011000UL

#define USART1_SR           (*(volatile uint32_t*)(USART1_BASE + 0x00UL))  // status register
#define USART1_DR           (*(volatile uint32_t*)(USART1_BASE + 0x04UL))  // data register
#define USART1_BRR          (*(volatile uint32_t*)(USART1_BASE + 0x08UL))  // baud rate register
#define USART1_CR1          (*(volatile uint32_t*)(USART1_BASE + 0x0CUL))  // control register 1


#define STK_CTRL            (*(volatile uint32_t*)0xE000E010)
#define STK_LOAD            (*(volatile uint32_t*)0xE000E014)
#define STK_VAL             (*(volatile uint32_t*)0xE000E018)
#define STK_CALIB           (*(volatile uint32_t*)0xE000E01C)

#define AIRCR               (*(volatile uint32_t*)0xE000ED0C)

// ── NVIC ───────────────────────────────────────────────────────────────────
// PM0214 §4.2.2 — write 1 to set-enable an IRQ; each register covers 32 IRQs
// IRQ N: register NVIC_ISER[N/32], bit N%32

#define NVIC_ISER0          (*(volatile uint32_t*)0xE000E100)  // IRQ0–31
#define NVIC_ISER1          (*(volatile uint32_t*)0xE000E104)  // IRQ32–63
#define NVIC_ISER2          (*(volatile uint32_t*)0xE000E108)  // IRQ64–95

// ── SPI ────────────────────────────────────────────────────────────────────
// RM0383 §20.5

// apb2
#define SPI1_BASE           0x40013000UL

// apb1
#define SPI2_BASE           0x40003800UL
#define SPI3_BASE           0x40003C00UL

// Generic base-relative SPI register access (RM0383 §20.5)
#define SPI_CR1(base)       (*(volatile uint32_t*)((base) + 0x00UL))  // control register 1
#define SPI_CR2(base)       (*(volatile uint32_t*)((base) + 0x04UL))  // control register 2
#define SPI_SR(base)        (*(volatile uint32_t*)((base) + 0x08UL))  // status register
#define SPI_DR(base)        (*(volatile uint32_t*)((base) + 0x0CUL))  // data register
#define SPI_DR_ADDR(base)   (base) + 0x0CUL                           // data register address

// SPI_CR1 bits (RM0383 §20.5.1)
#define SPI_CR1_CPHA        (1UL << 0)   // clock phase
#define SPI_CR1_CPOL        (1UL << 1)   // clock polarity
#define SPI_CR1_MSTR        (1UL << 2)   // master mode
#define SPI_CR1_BR_MASK     (7UL << 3)   // baud rate prescaler bits [5:3]: 000=÷2 ... 111=÷256
#define SPI_CR1_SPE         (1UL << 6)   // SPI enable
#define SPI_CR1_LSBFIRST    (1UL << 7)   // LSB first (0=MSB first)
#define SPI_CR1_SSI         (1UL << 8)   // internal slave select
#define SPI_CR1_SSM         (1UL << 9)   // software slave management
#define SPI_CR1_DFF         (1UL << 11)  // data frame format: 0=8-bit, 1=16-bit

// SPI_CR2 bits (RM0383 §20.5.2)
#define SPI_CR2_TXDMAEN     (1UL << 1)   // TX DMA enable
#define SPI_CR2_RXDMAEN     (1UL << 0)   // RX DMA enable

// SPI_SR bits (RM0383 §20.5.3)
#define SPI_SR_RXNE         (1UL << 0)   // receive buffer not empty
#define SPI_SR_TXE          (1UL << 1)   // transmit buffer empty
#define SPI_SR_BSY          (1UL << 7)   // busy flag

// ── DMA ────────────────────────────────────────────────────────────────────
// RM0383 §9

// both on AHB1
#define DMA1_BASE           0x40026000UL
#define DMA2_BASE           0x40026400UL

// DMA interrupt status registers (RM0383 §9.5.1)
#define DMA2_LISR           (*(volatile uint32_t*)(DMA2_BASE + 0x00UL))  // low interrupt status  (streams 0-3)
#define DMA2_HISR           (*(volatile uint32_t*)(DMA2_BASE + 0x04UL))  // high interrupt status (streams 4-7)
#define DMA2_LIFCR          (*(volatile uint32_t*)(DMA2_BASE + 0x08UL))  // low interrupt flag clear
#define DMA2_HIFCR          (*(volatile uint32_t*)(DMA2_BASE + 0x0CUL))  // high interrupt flag clear

// DMA2 Stream3 registers — SPI1_TX (RM0383 Table 28, §9.5.5)
#define DMA2_S3_BASE        (DMA2_BASE + 0x058UL)
#define DMA2_S3CR           (*(volatile uint32_t*)(DMA2_S3_BASE + 0x00UL))  // configuration
#define DMA2_S3NDTR         (*(volatile uint32_t*)(DMA2_S3_BASE + 0x04UL))  // number of data
#define DMA2_S3PAR          (*(volatile uint32_t*)(DMA2_S3_BASE + 0x08UL))  // peripheral address
#define DMA2_S3M0AR         (*(volatile uint32_t*)(DMA2_S3_BASE + 0x0CUL))  // memory 0 address

// DMA1 Stream4 registers — SPI2_TX (RM0383 Table 27, §9.5.5)
#define DMA1_S4_BASE        (DMA1_BASE + 0x070UL)
#define DMA1_S4CR           (*(volatile uint32_t*)(DMA1_S4_BASE + 0x00UL))
#define DMA1_S4NDTR         (*(volatile uint32_t*)(DMA1_S4_BASE + 0x04UL))
#define DMA1_S4PAR          (*(volatile uint32_t*)(DMA1_S4_BASE + 0x08UL))
#define DMA1_S4M0AR         (*(volatile uint32_t*)(DMA1_S4_BASE + 0x0CUL))

// DMA1 Stream3 registers — SPI2_RX (RM0383 Table 27, §9.5.5)
#define DMA1_S3_BASE        (DMA1_BASE + 0x058UL)
#define DMA1_S3CR           (*(volatile uint32_t*)(DMA1_S3_BASE + 0x00UL))
#define DMA1_S3NDTR         (*(volatile uint32_t*)(DMA1_S3_BASE + 0x04UL))
#define DMA1_S3PAR          (*(volatile uint32_t*)(DMA1_S3_BASE + 0x08UL))
#define DMA1_S3M0AR         (*(volatile uint32_t*)(DMA1_S3_BASE + 0x0CUL))

// DMA1 interrupt status registers (RM0383 §9.5.1)
#define DMA1_LISR           (*(volatile uint32_t*)(DMA1_BASE + 0x00UL))
#define DMA1_HISR           (*(volatile uint32_t*)(DMA1_BASE + 0x04UL))
#define DMA1_LIFCR          (*(volatile uint32_t*)(DMA1_BASE + 0x08UL))
#define DMA1_HIFCR          (*(volatile uint32_t*)(DMA1_BASE + 0x0CUL))

// DMA stream CR bits (RM0383 §9.5.5)
#define DMA_SCR_EN          (1UL << 0)   // stream enable
#define DMA_SCR_TCIE        (1UL << 4)   // transfer complete interrupt enable
#define DMA_SCR_DIR_M2P     (1UL << 6)   // direction: memory to peripheral
#define DMA_SCR_DIR_P2M     (0UL << 6)   // direction: peripheral to memory (default)
#define DMA_SCR_MINC        (1UL << 10)  // memory increment mode
#define DMA_SCR_CHSEL(ch)   ((uint32_t)(ch) << 25) // channel select bits [27:25]

// DMA2 LISR/LIFCR bit positions for stream 3 — SPI1_TX (RM0383 §9.5.1)
#define DMA2_LISR_TCIF3     (1UL << 27)
#define DMA2_LIFCR_CTCIF3   (1UL << 27)
#define SPI1_TRANSFER_DONE  (DMA2_LISR & DMA2_LISR_TCIF3)

// DMA1 HISR/HIFCR bit positions for stream 4 — SPI2_TX (RM0383 §9.5.2)
#define DMA1_HISR_TCIF4     (1UL << 5)
#define DMA1_HIFCR_CTCIF4   (1UL << 5)

// DMA1 LISR/LIFCR bit positions for stream 3 — SPI2_RX (RM0383 §9.5.1)
#define DMA1_LISR_TCIF3     (1UL << 27)
#define DMA1_LIFCR_CTCIF3   (1UL << 27)

// ── ST7735R command IDs ────────────────────────────────────────────────────
// ST7735R datasheet v1.4 §8

#define ST7735_NOP          0x00  // no operation
#define ST7735_SWRESET      0x01  // software reset
#define ST7735_RDDID        0x04  // read display ID
#define ST7735_RDDST        0x09  // read display status
#define ST7735_SLPIN        0x10  // sleep in
#define ST7735_SLPOUT       0x11  // sleep out — wait 120ms after
#define ST7735_PTLON        0x12  // partial mode on
#define ST7735_NORON        0x13  // normal display mode on
#define ST7735_INVOFF       0x20  // display inversion off
#define ST7735_INVON        0x21  // display inversion on
#define ST7735_GAMSET       0x26  // gamma curve select
#define ST7735_DISPOFF      0x28  // display off
#define ST7735_DISPON       0x29  // display on
#define ST7735_CASET        0x2A  // column address set — 4 bytes: XS_hi, XS_lo, XE_hi, XE_lo
#define ST7735_RASET        0x2B  // row address set    — 4 bytes: YS_hi, YS_lo, YE_hi, YE_lo
#define ST7735_RAMWR        0x2C  // memory write — pixel data follows
#define ST7735_RAMRD        0x2E  // memory read
#define ST7735_PTLAR        0x30  // partial area
#define ST7735_TEON         0x35  // tearing effect line
#define ST7735_MADCTL       0x36  // memory access control (scan order, RGB/BGR)
#define ST7735_COLMOD       0x3A  // pixel format — 0x05 = RGB565
#define ST7735_FRMCTR1      0xB1  // frame rate control — normal mode
#define ST7735_FRMCTR2      0xB2  // frame rate control — idle mode
#define ST7735_FRMCTR3      0xB3  // frame rate control — partial mode
#define ST7735_INVCTR       0xB4  // display inversion control
#define ST7735_PWCTR1       0xC0  // power control 1
#define ST7735_PWCTR2       0xC1  // power control 2
#define ST7735_PWCTR3       0xC2  // power control 3
#define ST7735_PWCTR4       0xC3  // power control 4
#define ST7735_PWCTR5       0xC4  // power control 5
#define ST7735_VMCTR1       0xC5  // VCOM control 1
#define ST7735_RDID1        0xDA  // read ID1
#define ST7735_RDID2        0xDB  // read ID2
#define ST7735_RDID3        0xDC  // read ID3
#define ST7735_GMCTRP1      0xE0  // positive gamma correction (16 bytes)
#define ST7735_GMCTRN1      0xE1  // negative gamma correction (16 bytes)
#define ST7735_PWCTR6       0xFC  // power control 6 — partial + idle mode

// packs 8-bit r,g,b (0-255 each) into RGB565 — evaluates at compile time for literal args
#define RGB565(r, g, b)     ((uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | (((b) & 0xF8) >> 3)))

// ── W5500 SPI frame ────────────────────────────────────────────────────────
// W5500 datasheet §2.2 — every transaction is [addr_hi][addr_lo][control][data...]
// control byte: BSB[7:3] block select | RWB[2] read/write | OM[1:0] operation mode

#define W5500_OM_VDM        0x00  // variable data mode — CS delimits the frame
#define W5500_OM_FDM1       0x01  // fixed 1-byte data
#define W5500_OM_FDM2       0x02  // fixed 2-byte data
#define W5500_OM_FDM4       0x03  // fixed 4-byte data

#define W5500_RWB_READ      0x00
#define W5500_RWB_WRITE     0x04

// block select values (already shifted into BSB position)
#define W5500_BSB_COMMON    (0x00UL << 3)
#define W5500_BSB_SREG(n)   (((((n) << 2) | 0x01UL)) << 3)  // socket n register block
#define W5500_BSB_STX(n)    (((((n) << 2) | 0x02UL)) << 3)  // socket n TX buffer
#define W5500_BSB_SRX(n)    (((((n) << 2) | 0x03UL)) << 3)  // socket n RX buffer

#define W5500_CTRL(bsb, rwb) ((uint8_t)((bsb) | (rwb) | W5500_OM_VDM))

// ── W5500 common registers ─────────────────────────────────────────────────
// W5500 datasheet §3.1

#define W5500_MR            0x0000  // mode
#define W5500_GAR           0x0001  // gateway IP        (4 bytes)
#define W5500_SUBR          0x0005  // subnet mask       (4 bytes)
#define W5500_SHAR          0x0009  // source MAC        (6 bytes)
#define W5500_SIPR          0x000F  // source IP         (4 bytes)
#define W5500_INTLEVEL      0x0013  // interrupt low level timer (2 bytes)
#define W5500_IR            0x0015  // interrupt
#define W5500_IMR           0x0016  // interrupt mask
#define W5500_SIR           0x0017  // socket interrupt
#define W5500_SIMR          0x0018  // socket interrupt mask
#define W5500_RTR           0x0019  // retry time        (2 bytes)
#define W5500_RCR           0x001B  // retry count
#define W5500_PHYCFGR       0x002E  // PHY configuration
#define W5500_VERSIONR      0x0039  // version — always reads 0x04

#define W5500_VERSION       0x04    // expected VERSIONR value

// MR bits (W5500 datasheet §3.1)
#define W5500_MR_RST        (1UL << 7)  // software reset — self-clearing
#define W5500_MR_WOL        (1UL << 5)  // wake on LAN
#define W5500_MR_PB         (1UL << 4)  // ping block
#define W5500_MR_PPPOE      (1UL << 3)  // PPPoE mode
#define W5500_MR_FARP       (1UL << 1)  // force ARP

// PHYCFGR bits (W5500 datasheet §3.1)
#define W5500_PHYCFGR_LNK   (1UL << 0)  // link up (read-only)
#define W5500_PHYCFGR_SPD   (1UL << 1)  // speed: 0=10M, 1=100M (read-only)
#define W5500_PHYCFGR_DPX   (1UL << 2)  // duplex: 0=half, 1=full (read-only)
#define W5500_PHYCFGR_RST   (1UL << 7)  // PHY reset — active low

// ── W5500 socket registers ─────────────────────────────────────────────────
// W5500 datasheet §3.2 — offsets within a socket's register block

#define W5500_Sn_MR         0x0000  // socket mode
#define W5500_Sn_CR         0x0001  // socket command
#define W5500_Sn_IR         0x0002  // socket interrupt
#define W5500_Sn_SR         0x0003  // socket status
#define W5500_Sn_PORT       0x0004  // source port       (2 bytes)
#define W5500_Sn_DHAR       0x0006  // dest MAC          (6 bytes)
#define W5500_Sn_DIPR       0x000C  // dest IP           (4 bytes)
#define W5500_Sn_DPORT      0x0010  // dest port         (2 bytes)
#define W5500_Sn_MSSR       0x0012  // max segment size  (2 bytes)
#define W5500_Sn_TOS        0x0015  // IP type of service
#define W5500_Sn_TTL        0x0016  // IP time to live
#define W5500_Sn_RXBUF_SIZE 0x001E  // RX buffer size in KB
#define W5500_Sn_TXBUF_SIZE 0x001F  // TX buffer size in KB
#define W5500_Sn_TX_FSR     0x0020  // TX free size      (2 bytes, read-only)
#define W5500_Sn_TX_RD      0x0022  // TX read pointer   (2 bytes, read-only)
#define W5500_Sn_TX_WR      0x0024  // TX write pointer  (2 bytes)
#define W5500_Sn_RX_RSR     0x0026  // RX received size  (2 bytes, read-only)
#define W5500_Sn_RX_RD      0x0028  // RX read pointer   (2 bytes)
#define W5500_Sn_RX_WR      0x002A  // RX write pointer  (2 bytes, read-only)
#define W5500_Sn_IMR        0x002C  // socket interrupt mask
#define W5500_Sn_FRAG       0x002D  // fragment offset   (2 bytes)
#define W5500_Sn_KPALVTR    0x002F  // keep-alive timer

// Sn_MR protocol values (W5500 datasheet §3.2)
#define W5500_Sn_MR_CLOSED  0x00
#define W5500_Sn_MR_TCP     0x01
#define W5500_Sn_MR_UDP     0x02
#define W5500_Sn_MR_MACRAW  0x04

// Sn_CR commands — register self-clears to 0x00 once the command is accepted
#define W5500_Sn_CR_OPEN      0x01
#define W5500_Sn_CR_LISTEN    0x02
#define W5500_Sn_CR_CONNECT   0x04
#define W5500_Sn_CR_DISCON    0x08
#define W5500_Sn_CR_CLOSE     0x10
#define W5500_Sn_CR_SEND      0x20
#define W5500_Sn_CR_SEND_MAC  0x21
#define W5500_Sn_CR_SEND_KEEP 0x22
#define W5500_Sn_CR_RECV      0x40

// Sn_SR status values
#define W5500_SOCK_CLOSED      0x00
#define W5500_SOCK_INIT        0x13
#define W5500_SOCK_LISTEN      0x14
#define W5500_SOCK_SYNSENT     0x15
#define W5500_SOCK_SYNRECV     0x16
#define W5500_SOCK_ESTABLISHED 0x17
#define W5500_SOCK_FIN_WAIT    0x18
#define W5500_SOCK_CLOSING     0x1A
#define W5500_SOCK_TIME_WAIT   0x1B
#define W5500_SOCK_CLOSE_WAIT  0x1C
#define W5500_SOCK_LAST_ACK    0x1D
#define W5500_SOCK_UDP         0x22
#define W5500_SOCK_MACRAW      0x42

// Sn_IR bits — write 1 to clear
#define W5500_Sn_IR_CON     (1UL << 0)  // connection established
#define W5500_Sn_IR_DISCON  (1UL << 1)  // FIN received
#define W5500_Sn_IR_RECV    (1UL << 2)  // data received
#define W5500_Sn_IR_TIMEOUT (1UL << 3)  // ARP/TCP timeout
#define W5500_Sn_IR_SENDOK  (1UL << 4)  // SEND command complete