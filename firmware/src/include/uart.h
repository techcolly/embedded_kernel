#pragma once
#include <stdint.h>

class Uart {
public:
    enum class Port   { USART1, USART2 };
    enum class Status { SUCCESS, FAIL };

    Uart(Port port);

    void clock_init();

    void    write_byte(uint8_t byte);
    void    write(const char* s);
    uint8_t read_byte();

    Status last_status() const;

private:
    uint32_t base_;
    Port     port_;
    Status   last_status_;

    static uint32_t port_to_base(Port port);
    uint32_t        port_to_pclk();
};
