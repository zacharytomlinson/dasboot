#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <avr/io.h>

bool nvm_flash_write_page(uint32_t flash_addr, const uint8_t page[PROGMEM_PAGE_SIZE]);
