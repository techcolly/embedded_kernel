#pragma once
#include <stdint.h>

extern volatile uint32_t millis_count;
extern volatile bool spi1_tx_done;
extern volatile bool spi2_tx_done;
extern volatile bool spi2_rx_done; 

#ifdef __cplusplus
void delay(volatile uint32_t ms) noexcept;
#else
void delay(volatile uint32_t ms);
#endif
