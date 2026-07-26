#include <stdint.h>
#include <stdbool.h>
#include "include/constants.h"
#include "include/shared.h"

extern uint32_t _estack;

extern uint32_t _sidata; // LMA: where .data initial values sit in flash
extern uint32_t _sdata;  // VMA: where .data must live at runtime (RAM start)
extern uint32_t _edata;  // VMA: RAM end of .data

extern uint32_t _sbss;
extern uint32_t _ebss;


volatile uint32_t millis_count;
volatile bool spi1_tx_done;


int main(void);

void Reset_Handler(void);
void Default_Handler(void);

// weak aliases: defining the handler anywhere else silently overrides (PM0214 §2.3.4)
// ── Core exceptions (ARM-defined, RM0383 Table 37 positions 1–15) ──────────
void NMI_Handler(void)            __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void)      __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler(void)      __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void)       __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void)     __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void)            __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void)       __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void)         __attribute__((weak, alias("Default_Handler")));

// ── Peripheral interrupts (STM32F411-specific, RM0383 Table 37 IRQ 0–85) ───
void WWDG_IRQHandler(void)                __attribute__((weak, alias("Default_Handler")));
void PVD_IRQHandler(void)                 __attribute__((weak, alias("Default_Handler")));
void TAMP_STAMP_IRQHandler(void)          __attribute__((weak, alias("Default_Handler")));
void RTC_WKUP_IRQHandler(void)            __attribute__((weak, alias("Default_Handler")));
void FLASH_IRQHandler(void)               __attribute__((weak, alias("Default_Handler")));
void RCC_IRQHandler(void)                 __attribute__((weak, alias("Default_Handler")));
void EXTI0_IRQHandler(void)               __attribute__((weak, alias("Default_Handler")));
void EXTI1_IRQHandler(void)               __attribute__((weak, alias("Default_Handler")));
void EXTI2_IRQHandler(void)               __attribute__((weak, alias("Default_Handler")));
void EXTI3_IRQHandler(void)               __attribute__((weak, alias("Default_Handler")));
void EXTI4_IRQHandler(void)               __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream0_IRQHandler(void)        __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream1_IRQHandler(void)        __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream2_IRQHandler(void)        __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream5_IRQHandler(void)        __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream6_IRQHandler(void)        __attribute__((weak, alias("Default_Handler")));
void ADC_IRQHandler(void)                 __attribute__((weak, alias("Default_Handler")));
void EXTI9_5_IRQHandler(void)             __attribute__((weak, alias("Default_Handler")));
void TIM1_BRK_TIM9_IRQHandler(void)      __attribute__((weak, alias("Default_Handler")));
void TIM1_UP_TIM10_IRQHandler(void)      __attribute__((weak, alias("Default_Handler")));
void TIM1_TRG_COM_TIM11_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIM1_CC_IRQHandler(void)            __attribute__((weak, alias("Default_Handler")));
void TIM2_IRQHandler(void)               __attribute__((weak, alias("Default_Handler")));
void TIM3_IRQHandler(void)               __attribute__((weak, alias("Default_Handler")));
void TIM4_IRQHandler(void)               __attribute__((weak, alias("Default_Handler")));
void I2C1_EV_IRQHandler(void)            __attribute__((weak, alias("Default_Handler")));
void I2C1_ER_IRQHandler(void)            __attribute__((weak, alias("Default_Handler")));
void I2C2_EV_IRQHandler(void)            __attribute__((weak, alias("Default_Handler")));
void I2C2_ER_IRQHandler(void)            __attribute__((weak, alias("Default_Handler")));
void SPI1_IRQHandler(void)               __attribute__((weak, alias("Default_Handler")));
void SPI2_IRQHandler(void)               __attribute__((weak, alias("Default_Handler")));
void USART1_IRQHandler(void)             __attribute__((weak, alias("Default_Handler")));
void USART2_IRQHandler(void)             __attribute__((weak, alias("Default_Handler")));
void EXTI15_10_IRQHandler(void)          __attribute__((weak, alias("Default_Handler")));
void RTC_Alarm_IRQHandler(void)          __attribute__((weak, alias("Default_Handler")));
void OTG_FS_WKUP_IRQHandler(void)        __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream7_IRQHandler(void)       __attribute__((weak, alias("Default_Handler")));
void SDIO_IRQHandler(void)               __attribute__((weak, alias("Default_Handler")));
void TIM5_IRQHandler(void)               __attribute__((weak, alias("Default_Handler")));
void SPI3_IRQHandler(void)               __attribute__((weak, alias("Default_Handler")));
void DMA2_Stream0_IRQHandler(void)       __attribute__((weak, alias("Default_Handler")));
void DMA2_Stream1_IRQHandler(void)       __attribute__((weak, alias("Default_Handler")));
void DMA2_Stream2_IRQHandler(void)       __attribute__((weak, alias("Default_Handler")));
void DMA2_Stream4_IRQHandler(void)       __attribute__((weak, alias("Default_Handler")));
void OTG_FS_IRQHandler(void)             __attribute__((weak, alias("Default_Handler")));
void DMA2_Stream5_IRQHandler(void)       __attribute__((weak, alias("Default_Handler")));
void DMA2_Stream6_IRQHandler(void)       __attribute__((weak, alias("Default_Handler")));
void DMA2_Stream7_IRQHandler(void)       __attribute__((weak, alias("Default_Handler")));
void USART6_IRQHandler(void)             __attribute__((weak, alias("Default_Handler")));
void I2C3_EV_IRQHandler(void)            __attribute__((weak, alias("Default_Handler")));
void I2C3_ER_IRQHandler(void)            __attribute__((weak, alias("Default_Handler")));
void FPU_IRQHandler(void)                __attribute__((weak, alias("Default_Handler")));
void SPI4_IRQHandler(void)               __attribute__((weak, alias("Default_Handler")));
void SPI5_IRQHandler(void)               __attribute__((weak, alias("Default_Handler")));

// actual functions with definitions

void SysTick_Handler(void) {
    millis_count++;
}

extern void uart_write_isr(const char* s);

void DMA2_Stream3_IRQHandler(void) { // SPI1 : ST7735R display
    while (SPI_SR(SPI1_BASE) & SPI_SR_BSY); // wait for last byte to finish shifting out before CS deassert
    GPIOA_BSRR = (1UL << 4);        // set PA4 : CS deassert
    DMA2_LIFCR = (1UL << 27);       // clear TCIF3
    DMA2_S3CR &= ~(1UL << 0);       // disable DMA
    spi1_tx_done = true;
    uart_write_isr("ISR\n");
}

void DMA1_Stream4_IRQHandler(void) { // SPI2 : W5500 ethernet, write direction
    GPIOB_BSRR = (1UL << 12);       // set PB12 : CS deassert
    DMA1_HIFCR = (1UL << 5);        // clear TCIF4
    DMA1_S4CR &= ~(1UL << 0);       // disable DMA
}

void DMA1_Stream3_IRQHandler(void) { // SPI2 : W5500 ethernet, read direction
    GPIOB_BSRR = (1UL << 12);       // set PB12 : CS deassert
    DMA1_LIFCR = (1UL << 27);       // clear TCIF3
    DMA1_S3CR &= ~(1UL << 0);       // disable DMA
}

extern void (*__init_array_start[])(void); // start of C++ constructor pointer table in flash
extern void (*__init_array_end[])(void);

// vector table — positions must match RM0383 Table 37 exactly (PM0214 §2.3.4)
// reserved slots are 0 (null pointer); hardware skips them
__attribute__((section(".isr_vector"))) const void *vector_table[] = {
    // ── Core ──────────────────────────────────────────────────────────────
    &_estack,                       // 0x000: initial stack pointer
    Reset_Handler,                  // 0x004: reset
    NMI_Handler,                    // 0x008: NMI
    HardFault_Handler,              // 0x00C: hard fault
    MemManage_Handler,              // 0x010: MPU fault
    BusFault_Handler,               // 0x014: bus fault
    UsageFault_Handler,             // 0x018: usage fault
    0,                              // 0x01C: reserved
    0,                              // 0x020: reserved
    0,                              // 0x024: reserved
    0,                              // 0x028: reserved
    SVC_Handler,                    // 0x02C: SVCall
    DebugMon_Handler,               // 0x030: debug monitor
    0,                              // 0x034: reserved
    PendSV_Handler,                 // 0x038: PendSV
    SysTick_Handler,                // 0x03C: SysTick
    // ── Peripheral IRQs (RM0383 Table 37) ─────────────────────────────────
    WWDG_IRQHandler,                // IRQ0:  watchdog
    PVD_IRQHandler,                 // IRQ1:  PVD via EXTI
    TAMP_STAMP_IRQHandler,          // IRQ2:  tamper / timestamp
    RTC_WKUP_IRQHandler,            // IRQ3:  RTC wakeup
    FLASH_IRQHandler,               // IRQ4:  flash global
    RCC_IRQHandler,                 // IRQ5:  RCC global
    EXTI0_IRQHandler,               // IRQ6:  EXTI line 0
    EXTI1_IRQHandler,               // IRQ7:  EXTI line 1
    EXTI2_IRQHandler,               // IRQ8:  EXTI line 2
    EXTI3_IRQHandler,               // IRQ9:  EXTI line 3
    EXTI4_IRQHandler,               // IRQ10: EXTI line 4
    DMA1_Stream0_IRQHandler,        // IRQ11: DMA1 stream 0
    DMA1_Stream1_IRQHandler,        // IRQ12: DMA1 stream 1
    DMA1_Stream2_IRQHandler,        // IRQ13: DMA1 stream 2
    DMA1_Stream3_IRQHandler,        // IRQ14: DMA1 stream 3
    DMA1_Stream4_IRQHandler,        // IRQ15: DMA1 stream 4
    DMA1_Stream5_IRQHandler,        // IRQ16: DMA1 stream 5
    DMA1_Stream6_IRQHandler,        // IRQ17: DMA1 stream 6
    ADC_IRQHandler,                 // IRQ18: ADC global
    0,                              // IRQ19: reserved (no CAN on F411)
    0,                              // IRQ20: reserved
    0,                              // IRQ21: reserved
    0,                              // IRQ22: reserved
    EXTI9_5_IRQHandler,             // IRQ23: EXTI lines 9..5
    TIM1_BRK_TIM9_IRQHandler,      // IRQ24: TIM1 break / TIM9 global
    TIM1_UP_TIM10_IRQHandler,      // IRQ25: TIM1 update / TIM10 global
    TIM1_TRG_COM_TIM11_IRQHandler, // IRQ26: TIM1 trigger+comm / TIM11 global
    TIM1_CC_IRQHandler,            // IRQ27: TIM1 capture/compare
    TIM2_IRQHandler,               // IRQ28: TIM2 global
    TIM3_IRQHandler,               // IRQ29: TIM3 global
    TIM4_IRQHandler,               // IRQ30: TIM4 global
    I2C1_EV_IRQHandler,            // IRQ31: I2C1 event
    I2C1_ER_IRQHandler,            // IRQ32: I2C1 error
    I2C2_EV_IRQHandler,            // IRQ33: I2C2 event
    I2C2_ER_IRQHandler,            // IRQ34: I2C2 error
    SPI1_IRQHandler,               // IRQ35: SPI1 global
    SPI2_IRQHandler,               // IRQ36: SPI2 global
    USART1_IRQHandler,             // IRQ37: USART1 global
    USART2_IRQHandler,             // IRQ38: USART2 global
    0,                             // IRQ39: reserved (no USART3 on F411)
    EXTI15_10_IRQHandler,          // IRQ40: EXTI lines 15..10
    RTC_Alarm_IRQHandler,          // IRQ41: RTC alarm via EXTI
    OTG_FS_WKUP_IRQHandler,        // IRQ42: USB OTG FS wakeup via EXTI
    0,                             // IRQ43: reserved
    0,                             // IRQ44: reserved
    0,                             // IRQ45: reserved
    0,                             // IRQ46: reserved
    DMA1_Stream7_IRQHandler,       // IRQ47: DMA1 stream 7
    0,                             // IRQ48: reserved (no FSMC on F411)
    SDIO_IRQHandler,               // IRQ49: SDIO global
    TIM5_IRQHandler,               // IRQ50: TIM5 global
    SPI3_IRQHandler,               // IRQ51: SPI3 global
    0,                             // IRQ52: reserved
    0,                             // IRQ53: reserved
    0,                             // IRQ54: reserved
    0,                             // IRQ55: reserved
    DMA2_Stream0_IRQHandler,       // IRQ56: DMA2 stream 0
    DMA2_Stream1_IRQHandler,       // IRQ57: DMA2 stream 1
    DMA2_Stream2_IRQHandler,       // IRQ58: DMA2 stream 2
    DMA2_Stream3_IRQHandler,       // IRQ59: DMA2 stream 3
    DMA2_Stream4_IRQHandler,       // IRQ60: DMA2 stream 4
    0,                             // IRQ61: reserved
    0,                             // IRQ62: reserved
    OTG_FS_IRQHandler,             // IRQ63: USB OTG FS global
    DMA2_Stream5_IRQHandler,       // IRQ64: DMA2 stream 5
    DMA2_Stream6_IRQHandler,       // IRQ65: DMA2 stream 6
    DMA2_Stream7_IRQHandler,       // IRQ66: DMA2 stream 7
    USART6_IRQHandler,             // IRQ67: USART6 global
    I2C3_EV_IRQHandler,            // IRQ68: I2C3 event
    I2C3_ER_IRQHandler,            // IRQ69: I2C3 error
    0,                             // IRQ70: reserved
    0,                             // IRQ71: reserved
    0,                             // IRQ72: reserved
    0,                             // IRQ73: reserved
    0,                             // IRQ74: reserved
    0,                             // IRQ75: reserved
    0,                             // IRQ76: reserved
    FPU_IRQHandler,                // IRQ77: FPU global
    0,                             // IRQ78: reserved
    0,                             // IRQ79: reserved
    SPI4_IRQHandler,               // IRQ80: SPI4 global
    SPI5_IRQHandler,               // IRQ81: SPI5 global
};

uint32_t clock_init(void) {

    FLASH_ACR &= ~FLASH_ACR_LATENCY_MASK; // clear bits of latency area
    FLASH_ACR |= FLASH_ACR_LATENCY_3WS; // set latency to 3 wait states

    FLASH_ACR |= FLASH_ACR_PRFTEN; // enable instruction prefetch
    FLASH_ACR |= FLASH_ACR_ICEN; // enable instruction cache 
    FLASH_ACR |= FLASH_ACR_DCEN; // enable data cache

    RCC_PLLCFGR &= ~(1UL << 22); // set PLLSRC to HSI clock
    RCC_PLLCFGR &= ~(0x7FFFUL); // clear PLLN and PLLM bits
    RCC_PLLCFGR &= ~(3UL << 16); // clear PLLP bits (is already correct after this)

    RCC_PLLCFGR |= 16UL; // set PLLM to 16 (default value of HSI; cancels arithmetically)
    RCC_PLLCFGR |= (200UL << 6); // set PLLN to 200
    
    RCC_CR |= RCC_CR_PLLON; // enable PLL

    while(!(RCC_CR & RCC_CR_PLLRDY)); // wait until high

    RCC_CFGR &= ~(0xFUL << 4); // clear AHB prescaler bits; already correct, system clock not divided

    RCC_CFGR &= ~RCC_CFGR_PPRE1_MASK; // clear APB1 prescaler bits
    RCC_CFGR |= RCC_CFGR_PPRE1_DIV2; // set APB1 prescaler to divide by 2

    RCC_CFGR &= ~RCC_CFGR_PPRE2_MASK; // clear APB2 prescaler bits; already correct, will divide by 1

    RCC_CFGR &= ~3UL; // clear lower 2 bits for switch
    RCC_CFGR |= RCC_CFGR_SW_PLL; // set SYSCLK to PLL

    while ((RCC_CFGR & RCC_CFGR_SWS_MASK) != RCC_CFGR_SWS_PLL); // wait until that happens

    return 0; // this can return some fail value later
}

void Reset_Handler(void)
{    
    clock_init();
    
    uint32_t *src;
    uint32_t *dst;

    // copy .data initial values from flash (LMA) to RAM (VMA)
    src = &_sidata;
    dst = &_sdata;

    while (dst < &_edata) {
        *dst = *src;
        dst++;
        src++;
    }

    // C standard requires zero-initialized globals — nothing guarantees RAM is clean on reset
    dst = &_sbss;

    while (dst < &_ebss) {
        *dst = 0;
        dst++;
    }

    STK_VAL = 0; // clear previous counter
    STK_LOAD = (100000 - 1); // Set to 100Mhz / 1000ms = 100000 - 1
    STK_CTRL = (0x00000007UL << 0); // enable, enable tick interrupt, clock source AHB
    
    AIRCR = (0x5FA00000UL) | (3 << 8); // set to PRIGROUP=3 + key

    NVIC_ISER0 |= (1UL << 14); // DMA1_Stream3 IRQ14 : SPI2 RX
    NVIC_ISER0 |= (1UL << 15); // DMA1_Stream4 IRQ15 : SPI2 TX
    NVIC_ISER1 |= (1UL << 27); // DMA2_Stream3 IRQ59 : SPI1 TX

    // call C++ global constructors before main()
    for (void (**f_ptr)(void) = __init_array_start; f_ptr < __init_array_end; f_ptr++) {
        (*f_ptr)();
    }

    main();

    while (1) {
    }
}

void Default_Handler(void)
{
    while (1) {
    }
}
