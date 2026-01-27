#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void twi0_init(uint32_t scl_hz);
bool twi0_address_only(uint8_t addr7);
bool twi0_write(uint8_t addr7, const uint8_t *data, size_t len);
bool twi0_read(uint8_t addr7, uint8_t *data, size_t len);

