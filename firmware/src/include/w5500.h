#pragma once
#include <stdint.h>
#include "spi.h"

class W5500 {
public:
    enum class Status { SUCCESS, FAIL };

    W5500(Spi& spi, uint32_t rst_pin);

    Status  init(const uint8_t* mac, const uint8_t* ip,
                 const uint8_t* subnet, const uint8_t* gateway);
    uint8_t version();
    bool    link_up();

    Status   listen(uint8_t sock, uint16_t port);
    uint8_t  socket_status(uint8_t sock);
    uint16_t available(uint8_t sock);
    uint16_t recv(uint8_t sock, uint8_t* buf, uint16_t len);
    uint16_t send(uint8_t sock, const uint8_t* buf, uint16_t len);
    Status   close(uint8_t sock);

    Status last_status() const;

private:
    Spi&     spi_;
    uint32_t rst_pin_;
    Status   last_status_;

    void reg_read(uint16_t addr, uint8_t bsb, uint8_t* buf, uint16_t len);
    void reg_write(uint16_t addr, uint8_t bsb, const uint8_t* buf, uint16_t len);

    uint8_t  read_u8(uint16_t addr, uint8_t bsb);
    void     write_u8(uint16_t addr, uint8_t bsb, uint8_t val);
    uint16_t read_u16(uint16_t addr, uint8_t bsb);
    void     write_u16(uint16_t addr, uint8_t bsb, uint16_t val);

    void command(uint8_t sock, uint8_t cmd);
    void wait_transfer();
};
