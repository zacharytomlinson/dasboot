#include "boot_eeprom.h"

#include "crc16.h"

#include <avr/eeprom.h>

static uint16_t boot_state_crc(const boot_state_t *st)
{
    return boot_crc16_compute(st, (uint16_t)(sizeof(*st) - sizeof(st->crc16)), (uint16_t)BOOT_CRC16_INIT);
}

static boot_state_t EEMEM ee_boot_state;

bool boot_eeprom_is_valid(const boot_state_t *st)
{
    if (st->crc16 != boot_state_crc(st)) {
        return false;
    }
    if (st->pending_slot != 0xFF && st->pending_slot > 1) {
        return false;
    }
    if (st->confirmed_slot > 1) {
        return false;
    }
    return true;
}

void boot_eeprom_load(boot_state_t *st)
{
    eeprom_read_block(st, &ee_boot_state, sizeof(*st));
    if (!boot_eeprom_is_valid(st)) {
        st->pending_slot = 0xFF;
        st->confirmed_slot = 0;
        st->attempts = 0;
        st->flags = 0;
        st->crc16 = boot_state_crc(st);
        eeprom_update_block(st, &ee_boot_state, sizeof(*st));
    }
}

void boot_eeprom_store(const boot_state_t *st)
{
    boot_state_t tmp = *st;
    tmp.crc16 = boot_state_crc(&tmp);
    eeprom_update_block(&tmp, &ee_boot_state, sizeof(tmp));
}
