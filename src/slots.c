#include "slots.h"

#include "crc16.h"

#include "drivers/w25q.h"

#include <avr/io.h>
#include <string.h>

uint32_t slot_base_addr(uint8_t slot)
{
    return (slot == 0) ? W25_SLOT_A_BASE : W25_SLOT_B_BASE;
}

uint32_t slot_image_base(uint8_t slot)
{
    return slot_base_addr(slot) + (uint32_t)sizeof(slot_header_t);
}

bool slot_read_header(uint8_t slot, slot_header_t *hdr)
{
    if (!hdr || slot > 1) {
        return false;
    }

    uint32_t base = slot_base_addr(slot);
    if (!w25q_read(base, (uint8_t *)hdr, (uint16_t)sizeof(*hdr))) {
        return false;
    }

    if (hdr->magic != SLOT_MAGIC || hdr->header_version != SLOT_HEADER_VERSION) {
        return false;
    }

    // CRC is computed as if `state` were READY, so firmware may later clear bits
    // (e.g., READY->CONFIRMED) without needing to update CRC (which would require 0->1 writes).
    slot_header_t tmp = *hdr;
    tmp.state = SLOT_STATE_READY;
    uint16_t expect = boot_crc16_compute(
            &tmp, (uint16_t)(sizeof(tmp) - sizeof(tmp.header_crc16)), (uint16_t)BOOT_CRC16_INIT);
    if (expect != hdr->header_crc16) {
        return false;
    }

    // Accept READY or any later (bit-cleared) state such as CONFIRMED.
    if (hdr->state > SLOT_STATE_READY) {
        return false;
    }

    uint16_t max_len = (uint16_t)(PROGMEM_SIZE - APP_START_ADDR);
    if (hdr->image_len == 0 || hdr->image_len > (uint32_t)max_len) {
        return false;
    }

    // Optional: sanity check that header fits within slot.
    if (hdr->image_len > (uint32_t)(W25_SLOT_SIZE - (uint32_t)sizeof(*hdr))) {
        return false;
    }

    return true;
}
