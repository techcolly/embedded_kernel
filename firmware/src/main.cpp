#include "include/uart.h"
#include "include/spi.h"
#include "include/constants.h"
#include "include/shared.h"
#include "include/display.h"

Uart uart(Uart::Port::USART2);
Spi spi1(Spi::Port::SPI1);
Spi spi2(Spi::Port::SPI2);

extern "C" void uart_write_isr(const char* s) {
    uart.write(s);
}

void delay(volatile uint32_t ms) noexcept {
    uint32_t start = millis_count;
    while ((millis_count - start) < ms);
}

int main(void) {
    delay(500);
    Display display(spi1, GPIOB_BASE, 4, GPIOB_BASE, 3);
    uart.write("before init\n");
    display.init();
    uart.write("after init\n");
    delay(150);
    display.fill(RGB565(255, 255, 0));
    uart.write("after fill\n");
    delay(1000);
    display.displayTestPattern();
    uart.write("after test pattern\n");
    delay(15000);
    while(1) {};
    return 0;
}
