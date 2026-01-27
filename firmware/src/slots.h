#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "boot_contract.h"

uint32_t slot_base_addr(uint8_t slot);
bool slot_read_header(uint8_t slot, slot_header_t *hdr);
uint32_t slot_image_base(uint8_t slot);

