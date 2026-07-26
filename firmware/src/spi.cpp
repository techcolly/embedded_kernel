#include "include/spi.h"
#include "include/constants.h"

[[nodiscard]] uint32_t Spi::port_to_base(Port port) noexcept {
    switch (port) {
        case Spi::Port::SPI1: return SPI1_BASE;
        case Spi::Port::SPI2: return SPI2_BASE;
        case Spi::Port::SPI3: return SPI3_BASE;
        default:              return 0;
    }
}

[[nodiscard]] Spi::Status Spi::spi__last_status() const noexcept {
    return last_status_;
}

void Spi::spi__clock_init() noexcept {
    switch (this->port_) {
        case Spi::Port::SPI1: // enable SPI1 clock (RCC_APB2ENR)
            RCC_APB2ENR |= RCC_APB2ENR_SPI1EN;
            break;
        case Spi::Port::SPI2: // enable SPI2 clock (RCC_APB1ENR)
            RCC_APB1ENR |= RCC_APB1ENR_SPI2EN;
            break;
        case Spi::Port::SPI3: // enable SPI3 clock (RCC_APB1ENR)
            RCC_APB1ENR |= RCC_APB1ENR_SPI3EN;
            break;
        default: last_status_ = Status::FAIL; return;
    }
    last_status_ = Status::SUCCESS;
}

// SPI1: PA5=SCK, PA6=MISO, PA7=MOSI — AF5 (DS10314 Table 9)
// SPI2: PB13=SCK, PB14=MISO, PB15=MOSI — AF5 (DS10314 Table 9)

Spi::Spi(Port port)
    : base_(port_to_base(port)), port_(port), last_status_(Status::SUCCESS), cs_gpio_base_(0), cs_pin_(0) {

    spi__clock_init();
    
    switch(this->port_) {
        case Spi::Port::SPI1: { // ST7735R display
            RCC_AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
            RCC_AHB1ENR |= RCC_AHB1ENR_DMA2EN;

            GPIOA_MODER &= ~(0x3FUL << 10); // clear PA5, 6, 7

            GPIOA_MODER |= (GPIO_MODER_AF << 10);
            GPIOA_MODER |= (GPIO_MODER_AF << 12);
            GPIOA_MODER |= (GPIO_MODER_AF << 14);
            

            GPIOA_AFRL &= ~(0xFUL << 20);   // clear PA5 AF
            GPIOA_AFRL |=  (5UL << 20);
            GPIOA_AFRL &= ~(0xFUL << 24);   // clear PA6 AF
            GPIOA_AFRL |=  (5UL << 24);
            GPIOA_AFRL &= ~(0xFUL << 28);   // clear PA7 AF
            GPIOA_AFRL |=  (5UL << 28);

            GPIOA_OSPEEDR &= ~(0x3FUL << 10);
            GPIOA_OSPEEDR |= (0x3FUL << 10);

            SPI_CR1(this->base_) |= ((1UL << 2) | (3UL << 8)); // set software NSS + master mode
            SPI_CR1(this->base_) &= ~((1UL << 7) | (1UL << 11)); // clear DFF and MSB bits
            SPI_CR1(this->base_) |= (0b010UL << 3); // prescaler: divide by 8: ST7735R max is ~15Mhz
            SPI_CR1(this->base_) |= (0UL << 0); // CLK 0 then idle, second clock transition is first data capture edge
            SPI_CR1(this->base_) |= (1 << 6); // enable SPI1
            SPI_CR2(this->base_) |= SPI_CR2_TXDMAEN;

            DMA2_S3CR |= DMA_SCR_CHSEL(3); // channel 3; SPI1_TX
            DMA2_S3CR |= (0xAUL << 1); // enable TEIE, TCIE
            DMA2_S3PAR = SPI_DR_ADDR(SPI1_BASE);
            DMA2_S3CR |= DMA_SCR_DIR_M2P;       // memory to peripheral
            DMA2_S3CR |= DMA_SCR_MINC;          // increment memory address per byte

            this->cs_gpio_base_ = GPIOA_BASE;
            this->cs_pin_ = 4UL;

            GPIOA_MODER &= ~(0x3UL << 8);           // clear PA4
            GPIOA_MODER |= (GPIO_MODER_OUTPUT << 8); // PA4 as output
            GPIOA_BSRR = (1UL << 4);              // PA4 high: CS deasserted

            break;
        }

        case Spi::Port::SPI2: { // W5500 ethernet
            RCC_AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
            RCC_AHB1ENR |= RCC_AHB1ENR_DMA1EN;

            GPIOB_MODER &= ~(0x3FUL << 26); // clear PB13, 14, 15

            GPIOB_MODER |= (GPIO_MODER_AF << 26);
            GPIOB_MODER |= (GPIO_MODER_AF << 28);
            GPIOB_MODER |= (GPIO_MODER_AF << 30);

            GPIOB_AFRH &= ~(0xFUL << 20);   // clear PB13 AF
            GPIOB_AFRH |=  (5UL << 20);
            GPIOB_AFRH &= ~(0xFUL << 24);   // clear PB14 AF
            GPIOB_AFRH |=  (5UL << 24);
            GPIOB_AFRH &= ~(0xFUL << 28);   // clear PB15 AF
            GPIOB_AFRH |=  (5UL << 28);

            GPIOB_OSPEEDR &= ~(0x3FUL << 26);
            GPIOB_OSPEEDR |= (0x3FUL << 26);

            SPI_CR1(this->base_) |= ((1UL << 2) | (3UL << 8)); // set software NSS + master mode
            SPI_CR1(this->base_) &= ~((1UL << 7) | (1UL << 11)); // clear DFF and MSB bits
            SPI_CR1(this->base_) |= (0b000UL << 3); // prescaler: divide by 2; W5500 min is 33Mhz
            SPI_CR1(this->base_) |= (0UL << 0); // according to the manual it supports anything
            SPI_CR1(this->base_) |= (1 << 6); // enable SPI2

            SPI_CR2(this->base_) |= SPI_CR2_TXDMAEN;
            SPI_CR2(this->base_) |= SPI_CR2_RXDMAEN;

            DMA1_S4CR |= DMA_SCR_CHSEL(0);      // channel 0: SPI2_TX
            DMA1_S4CR |= (0xAUL << 1);          // enable TEIE, TCIE
            DMA1_S4CR |= DMA_SCR_DIR_M2P;       // memory to peripheral
            DMA1_S4CR |= DMA_SCR_MINC;          // increment memory address per byte
            DMA1_S4PAR = SPI_DR_ADDR(SPI2_BASE);

            DMA1_S3CR |= DMA_SCR_CHSEL(0);      // channel 0: SPI2_RX
            DMA1_S3CR |= (0xAUL << 1);          // enable TEIE, TCIE
            DMA1_S3CR |= DMA_SCR_DIR_P2M;       // peripheral to memory
            DMA1_S3CR |= DMA_SCR_MINC;          // increment memory address per byte
            DMA1_S3PAR = SPI_DR_ADDR(SPI2_BASE);

            this->cs_gpio_base_ = GPIOB_BASE;
            this->cs_pin_ = 12UL;

            GPIOB_MODER &= ~(0x3UL << 24);            // clear PB12
            GPIOB_MODER |= (GPIO_MODER_OUTPUT << 24); // PB12 as output
            GPIOB_BSRR = (1UL << 12);               // PB12 high: CS deasserted

            break;
        }

        case Spi::Port::SPI3: { // reserved
            break;
        }
    }
}

void Spi::spi__transfer(const uint8_t* tx, uint8_t* rx, uint16_t len) noexcept {
    if (!len || (!tx && !rx) || (tx && rx)) {
        this->last_status_ = Spi::Status::FAIL; // add more detailed ones later
        return;
    }

    if (tx) {

        if (this->port_ == Spi::Port::SPI1) {
            DMA2_S3M0AR = (uint32_t)tx;
            DMA2_S3NDTR = len;
            GPIOA_BSRR = (1UL << (4 + 16)); // reset PA4 : NSS active low
            DMA2_S3CR |= (1UL << 0);            // enable DMA 
            
        } else if (this->port_ == Spi::Port::SPI2) {
            DMA1_S4M0AR = (uint32_t)tx;
            DMA1_S4NDTR = len;
            GPIOB_BSRR = (1UL << (12 + 16));  // reset PB12 : NSS active low
            DMA1_S4CR |= (1UL << 0);            // enable DMA
        } else {
            return;
        }

    } else {
        if (this->port_ == Spi::Port::SPI1) {
            this->last_status_ = Spi::Status::FAIL; // invalid for this case
            return;
        } else if (this->port_ == Spi::Port::SPI2) {
            DMA1_S3M0AR = (uint32_t)rx;
            DMA1_S3NDTR = len;
            GPIOB_BSRR = (1UL << (12 + 16));   // reset PB12 : NSS active low
            DMA1_S3CR |= (1UL << 0);            // enable DMA
        } else {
            return;
        }
    }
    
}
