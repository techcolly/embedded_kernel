#pragma once
#include <stdint.h>
#include "spi.h"

class Display {
public:
    enum class Status { SUCCESS, FAIL };

    Display(Spi& spi,
            uint32_t dc_gpio_base,  uint32_t dc_pin,
            uint32_t rst_gpio_base, uint32_t rst_pin);

    void   init();

    void   write_command(uint8_t cmd);
    void   write_data(const uint8_t* data, uint16_t len);
    void   write_data(uint8_t data);
    void   fill(uint16_t color);
    void   draw_pixel(uint8_t x, uint8_t y, uint16_t color);

    void   displayTestPattern();

    Status last_status() const;

private:
    Spi&     spi_;
    uint32_t dc_gpio_base_;
    uint32_t dc_pin_;
    uint32_t rst_gpio_base_;
    uint32_t rst_pin_;
    Status   last_status_;
    
    void defaultWindowSize();
    void wait_transfer();
};
