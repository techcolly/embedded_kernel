#include "include/w5500.h"
#include "include/constants.h"
#include "include/shared.h"

W5500::W5500(Spi& spi, uint32_t rst_pin)
    : spi_(spi), rst_pin_(rst_pin), last_status_(Status::SUCCESS) {

    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOBEN;

    GPIOB_MODER &= ~(0x3UL             << GPIO_MODER_POS(rst_pin_)); // RST : clear mode
    GPIOB_MODER |=  (GPIO_MODER_OUTPUT << GPIO_MODER_POS(rst_pin_)); // RST as output
    GPIOB_BSRR = GPIO_BSRR_SET(rst_pin_);                            // RST high: not in reset
}

[[nodiscard]] W5500::Status W5500::last_status() const noexcept {
    return last_status_;
}

void W5500::wait_transfer() noexcept {
    while (!spi2_rx_done);
    spi2_rx_done = false;
}

// [addr_hi][addr_lo][control] then len data bytes — CS must stay low for the whole frame
void W5500::reg_read(uint16_t addr, uint8_t bsb, uint8_t* buf, uint16_t len) noexcept {
    (void)addr; (void)bsb; (void)buf; (void)len;

    uint8_t sendingBuffer[len + 3];  // optimize later

    sendingBuffer[0] = static_cast<uint8_t>(addr >> 8);
    sendingBuffer[1] = static_cast<uint8_t>(addr);
    sendingBuffer[2] = bsb;

    this->spi_.spi__transfer(sendingBuffer, buf, len + 3, true);
    this->wait_transfer();
}

void W5500::reg_write(uint16_t addr, uint8_t bsb, const uint8_t* buf, uint16_t len) noexcept {
    (void)addr; (void)bsb; (void)buf; (void)len;

    uint8_t sendingBuffer[len + 3]; // optimize later
    uint8_t dummyRx[len + 3]; // optimize later

    sendingBuffer[0] = static_cast<uint8_t>(addr >> 8);
    sendingBuffer[1] = static_cast<uint8_t>(addr);
    sendingBuffer[2] = bsb | W5500_RWB_WRITE;

    for (int i = 0; i < len; i++) {
        sendingBuffer[i + 3] = buf[i]; // optimize later
    }

    this->spi_.spi__transfer(sendingBuffer, dummyRx, len + 3, true);
    this->wait_transfer();

}

[[nodiscard]] uint8_t W5500::read_u8(uint16_t addr, uint8_t bsb) noexcept {
    
    uint8_t tmp_buf[4];
    this->reg_read(addr, bsb, tmp_buf, 1);

    return tmp_buf[3];
}

void W5500::write_u8(uint16_t addr, uint8_t bsb, uint8_t val) noexcept {
    (void)addr; (void)bsb; (void)val;
    reg_write(addr, bsb, &val, 1);
}

// 16-bit registers are big-endian on the wire
[[nodiscard]] uint16_t W5500::read_u16(uint16_t addr, uint8_t bsb) noexcept {

    uint8_t tmp_buf[5];
    this->reg_read(addr, bsb, tmp_buf, 2);

    return (uint16_t)(((uint16_t)tmp_buf[3] << 8) | ((uint16_t)tmp_buf[4]));
}

void W5500::write_u16(uint16_t addr, uint8_t bsb, uint16_t val) noexcept {
    reg_write(addr, bsb, (const uint8_t*)&val, 2);
}

// Sn_CR self-clears once accepted — poll until it reads back 0
void W5500::command(uint8_t sock, uint8_t cmd) noexcept {
    (void)sock; (void)cmd;
}

[[nodiscard]] uint8_t W5500::version() noexcept {
    uint8_t v = read_u8(W5500_VERSIONR, W5500_BSB_COMMON);
    return (v == 0x04) ? v : 0; // if this is wrong there's an issue
}

[[nodiscard]] bool W5500::link_up() noexcept {
    return read_u8(W5500_PHYCFGR, W5500_BSB_COMMON);
}

W5500::Status W5500::init(const uint8_t* mac, const uint8_t* ip,
                          const uint8_t* subnet, const uint8_t* gateway) noexcept {
    (void)mac; (void)ip; (void)subnet; (void)gateway;

    GPIOB_BSRR = GPIO_BSRR_RESET(this->rst_pin_); // RST low: in reset
    delay(5); // need to wait at least 500us
    GPIOB_BSRR = GPIO_BSRR_SET(this->rst_pin_); // RST high

    delay(100); // stuff needs to stablilize


    return Status::SUCCESS;
}

W5500::Status W5500::listen(uint8_t sock, uint16_t port) noexcept {
    (void)sock; (void)port;
    return Status::SUCCESS;
}

[[nodiscard]] uint8_t W5500::socket_status(uint8_t sock) noexcept {
    (void)sock;
    return W5500_SOCK_CLOSED;
}

[[nodiscard]] uint16_t W5500::available(uint8_t sock) noexcept {
    (void)sock;
    return 0;
}

uint16_t W5500::recv(uint8_t sock, uint8_t* buf, uint16_t len) noexcept {
    (void)sock; (void)buf; (void)len;
    return 0;
}

uint16_t W5500::send(uint8_t sock, const uint8_t* buf, uint16_t len) noexcept {
    (void)sock; (void)buf; (void)len;
    return 0;
}

W5500::Status W5500::close(uint8_t sock) noexcept {
    (void)sock;
    return Status::SUCCESS;
}
