#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct __attribute__((packed)) {
    uint8_t pending_slot;   // 0=A, 1=B, 0xFF=none
    uint8_t confirmed_slot; // 0=A, 1=B
    uint8_t attempts;       // boot attempts while waiting confirm
    uint8_t flags;          // see BOOTF_*
    uint16_t crc16;         // CRC-CCITT over all previous bytes
} boot_state_t;

enum {
    BOOTF_APPLY_PENDING = 0x01, // app requests bootloader to apply pending_slot
    BOOTF_WAIT_CONFIRM = 0x02,  // bootloader has applied; app must confirm
};

void boot_eeprom_load(boot_state_t *st);
void boot_eeprom_store(const boot_state_t *st);
bool boot_eeprom_is_valid(const boot_state_t *st);

