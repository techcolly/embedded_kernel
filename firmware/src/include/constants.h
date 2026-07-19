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

// SPI2 pin assignments on GPIOB (DS10314 Table 9)
#define SPI2_SCK_PIN        13U   // PB13 AF5
#define SPI2_MISO_PIN       14U   // PB14 AF5
#define SPI2_MOSI_PIN       15U   // PB15 AF5

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

// DMA1 HISR/HIFCR bit positions for stream 4 — SPI2_TX (RM0383 §9.5.2)
#define DMA1_HISR_TCIF4     (1UL << 5)
#define DMA1_HIFCR_CTCIF4   (1UL << 5)

// DMA1 LISR/LIFCR bit positions for stream 3 — SPI2_RX (RM0383 §9.5.1)
#define DMA1_LISR_TCIF3     (1UL << 27)
#define DMA1_LIFCR_CTCIF3   (1UL << 27)