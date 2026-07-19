#pragma once
#include <stdint.h>

 // needs abstraction: display on spi1, ethernet on spi2
class Spi {
public:
    enum class Port   { SPI1, SPI2, SPI3 };
    enum class Status { SUCCESS, FAIL };

    Spi(Port port);

    void clock_init();

    void transfer(const uint8_t* tx, uint8_t* rx, uint16_t len);

    Status last_status() const;

private:
    Port     port_;
    Status   last_status_;

    uint32_t base_;
    uint32_t cs_gpio_base_;
    uint32_t cs_pin_;
    
    static uint32_t port_to_base(Port port);
};
