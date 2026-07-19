#include "include/uart.h"
#include "include/constants.h"
#include "shared.h"

void delay(volatile uint32_t ms) noexcept {
    uint32_t start = millis_count;
    while ((millis_count - start) < ms);
}

int main(void)
{     
    Uart uart(Uart::Port::USART2);
    uart.clock_init();

    while (1) {
        uart.write("Test ");
        delay(5000);
    }

    // write to uart after a 5000ms delay to test
    return 0;
}
