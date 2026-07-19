#include "include/uart.h"
#include "include/constants.h"

[[nodiscard]] uint32_t Uart::port_to_base(Port port) noexcept {
    switch (port) {
        case Uart::Port::USART1: return USART1_BASE;
        case Uart::Port::USART2: return USART2_BASE;
        default:                 return 0;
    }
}

[[nodiscard]] uint32_t Uart::port_to_pclk() noexcept {
    switch(this->port_) {
        case Uart::Port::USART1: return 100'000'000UL;
        case Uart::Port::USART2: return  50'000'000UL;
        default:                 return 0;
    }
}

[[nodiscard]] Uart::Status Uart::last_status() const {
    return last_status_;
}

void Uart::clock_init() noexcept {
    switch(this->port_) {
        case Uart::Port::USART1: RCC_APB2ENR |= RCC_APB2ENR_USART1EN; break;
        case Uart::Port::USART2: RCC_APB1ENR |= RCC_APB1ENR_USART2EN; break;
        default: last_status_ = Status::FAIL; return;
    }
    last_status_ = Status::SUCCESS;
}

Uart::Uart(Port port)
    : base_(port_to_base(port)), port_(port), last_status_(Status::SUCCESS) {
    
    clock_init();

    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    switch(this->port_) {
        case Uart::Port::USART2: {
            GPIOA_MODER &= ~(0x3UL << 4);   // clear PA2
            GPIOA_MODER |=  (GPIO_MODER_AF << 4);
            GPIOA_MODER &= ~(0x3UL << 6);   // clear PA3
            GPIOA_MODER |=  (GPIO_MODER_AF << 6);

            GPIOA_AFRL &= ~(0xFUL << 8);    // clear PA2 AF
            GPIOA_AFRL |=  (GPIO_AF7 << 8);
            GPIOA_AFRL &= ~(0xFUL << 12);   // clear PA3 AF
            GPIOA_AFRL |=  (GPIO_AF7 << 12);

            USART2_CR1 &= ~USART_CR1_OVER8;

            // USARTDIV = 50MHz / (16 * 115200) = 27.127 → mantissa=27, fraction=0.127*16=2
            USART2_BRR = USART_BRR_MANTISSA(27) | USART_BRR_FRACTION(2);

            USART2_CR1 |= USART_CR1_TE | USART_CR1_RE;
            USART2_CR1 |= USART_CR1_UE;
            break;
        }

        case Uart::Port::USART1: {
            GPIOA_MODER &= ~(0x3UL << 18);  // clear PA9
            GPIOA_MODER |=  (GPIO_MODER_AF << 18);
            GPIOA_MODER &= ~(0x3UL << 20);  // clear PA10
            GPIOA_MODER |=  (GPIO_MODER_AF << 20);

            GPIOA_AFRH &= ~(0xFUL << 4);    // clear PA9 AF
            GPIOA_AFRH |=  (GPIO_AF7 << 4);
            GPIOA_AFRH &= ~(0xFUL << 8);    // clear PA10 AF
            GPIOA_AFRH |=  (GPIO_AF7 << 8);

            USART1_CR1 &= ~USART_CR1_OVER8;

            // USARTDIV = 100MHz / (16 * 115200) = 54.25 → mantissa=54, fraction=0.25*16=4
            USART1_BRR = USART_BRR_MANTISSA(54) | USART_BRR_FRACTION(4);

            USART1_CR1 |= USART_CR1_TE | USART_CR1_RE;
            USART1_CR1 |= USART_CR1_UE;
            break;
        }
    }
}

void Uart::write_byte(uint8_t byte) noexcept {
    volatile uint32_t timer = 37500000UL;  // replace with real timer later

    while (!(USART_SR(this->base_) & USART_SR_TXE)) {
        if (!(--timer)) { this->last_status_ = Status::FAIL; return; }
    }

    USART_DR(this->base_) = byte; // add error checking later
    this->last_status_ = Status::SUCCESS;
}

void Uart::write(const char* s) noexcept {
    while (*s) {
        write_byte(static_cast<uint8_t>(*s++));
        if (this->last_status_ == Status::FAIL) return;
    }
    this->last_status_ = Status::SUCCESS;
}

[[nodiscard]] uint8_t Uart::read_byte() noexcept {
    volatile uint32_t timer = 37500000UL; // replace with real timer later

    while (!(USART_SR(this->base_) & USART_SR_RXNE)) {
        if (!(--timer)) { this->last_status_ = Status::FAIL; return 0; }
    }

    this->last_status_ = Status::SUCCESS;
    return static_cast<uint8_t>(USART_DR(this->base_));
}
