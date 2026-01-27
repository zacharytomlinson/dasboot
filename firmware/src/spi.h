#pragma once

#include <stdint.h>

void spi0_init(void);
uint8_t spi0_transfer(uint8_t byte);
void spi0_transfer_buf(const uint8_t *tx, uint8_t *rx, uint16_t len);

