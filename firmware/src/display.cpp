#include "include/display.h"
#include "include/constants.h"
#include "include/shared.h"

Display::Display(Spi& spi,
                 uint32_t dc_gpio_base,  uint32_t dc_pin,
                 uint32_t rst_gpio_base, uint32_t rst_pin)
    : spi_(spi),
      dc_gpio_base_(dc_gpio_base),   dc_pin_(dc_pin),
      rst_gpio_base_(rst_gpio_base), rst_pin_(rst_pin),
      last_status_(Status::SUCCESS) {

    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOBEN;

    GPIOB_MODER &= ~(0x3UL << (rst_pin_ * 2));
    GPIOB_MODER |=  (GPIO_MODER_OUTPUT << (rst_pin_ * 2));
    GPIOB_BSRR = (1UL << rst_pin_);             // RST high: not in reset

    GPIOB_MODER &= ~(0x3UL << (dc_pin_ * 2));
    GPIOB_MODER |=  (GPIO_MODER_OUTPUT << (dc_pin_ * 2));
}

[[nodiscard]] Display::Status Display::last_status() const noexcept {
    return last_status_;
}

void Display::wait_transfer() noexcept {
    while (!spi1_tx_done);
    spi1_tx_done = false;
}

void Display::write_command(uint8_t cmd) noexcept {
    GPIOB_BSRR = (1UL << (dc_pin_ + 16U));

    this->spi_.spi__transfer(&cmd, nullptr, 1);
    this->wait_transfer();
}

void Display::write_data(const uint8_t* data, uint16_t len) noexcept {
    GPIOB_BSRR = (1UL << dc_pin_);

    this->spi_.spi__transfer(data, nullptr, len);
    this->wait_transfer();
}

void Display::write_data(const uint8_t data) noexcept { // overloaded version
    GPIOB_BSRR = (1UL << dc_pin_);

    this->spi_.spi__transfer(&data, nullptr, 1);
    this->wait_transfer();
}

void Display::defaultWindowSize() noexcept {
    this->write_command(ST7735_CASET);
    this->write_data((const uint8_t*)(const uint8_t[]){0x00, 0x02, 0x00, 0x81}, 4);  // columns 2–129

    this->write_command(ST7735_RASET);
    this->write_data((const uint8_t*)(const uint8_t[]){0x00, 0x03, 0x00, 0x82}, 4);  // rows 3–130
}

void Display::init() noexcept {
    GPIOB_BSRR = (1UL << (rst_pin_ + 16U)); // pull RST pin low
    delay(25);

    GPIOB_BSRR = (1UL << rst_pin_); // pull RST pin high
    delay(25);

    this->write_command(ST7735_SWRESET);
    delay(150);

    this->write_command(ST7735_SLPOUT);
    delay(150);

    this->write_command(ST7735_COLMOD);
    this->write_data(0x05); // RGB565

    this->write_command(ST7735_MADCTL);
    this->write_data(0x08); // BGR ; change later if wrong

    this->write_command(ST7735_TEON); // looks cool
    this->write_data((uint8_t)0x01);

    this->write_command(ST7735_DISPON);
}

void Display::fill(uint16_t color) noexcept {
    this->defaultWindowSize();
    this->write_command(ST7735_RAMWR);
    static uint8_t fillArr[128 * 128 * 2] = {0}; // TODO: optimize — 32KB static buffer, sends in one shot

    for (int i = 0; i < (128 * 128 * 2); i += 2) {
        fillArr[i] = (uint8_t)(color >> 8);
        fillArr[i + 1] = (uint8_t)(color & 0x00FF);
    }

    this->write_data(fillArr, 128 * 128 * 2);
}

void Display::draw_pixel(uint8_t x, uint8_t y, uint16_t color) noexcept {
    uint8_t caset[4] = {0x00, (uint8_t)(x+2), 0x00, (uint8_t)(x+2)};
    uint8_t raset[4] = {0x00, (uint8_t)(y+1), 0x00, (uint8_t)(y+1)};

    this->write_command(ST7735_CASET);
    this->write_data(caset, 4);

    this->write_command(ST7735_RASET);
    this->write_data(raset, 4);

    this->write_command(ST7735_RAMWR);

    uint8_t pixel[2] = {(uint8_t)(color >> 8), (uint8_t)(color & 0xFF)};
    this->write_data(pixel, 2);
}   

void Display::displayTestPattern() noexcept {
    // TODO: slow — 7 SPI transactions per pixel. build one buffer and stream it like fill() does.
    uint16_t colors[3] = {0b1111100000000000, 0b0000011111100000, 0b0000000000011111};

    for (int y = 0; y < 128; y++) {
        for (int x = 0; x < 128; x++) {
            draw_pixel(x, y, colors[((y / 16) + (x / 16)) % 3]);
        }
    }

}