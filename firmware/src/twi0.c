#include "twi0.h"

#include <avr/io.h>

static bool twi0_wait(uint8_t mask)
{
    // Simple bounded wait to avoid hanging forever.
    // At 20 MHz, this is plenty for 100-400 kHz I2C transactions.
    for (uint32_t i = 0; i < 500000; i++) {
        uint8_t status = TWI0.MSTATUS;
        if ((status & (TWI_BUSERR_bm | TWI_ARBLOST_bm)) != 0) {
            return false;
        }
        if ((status & mask) != 0) {
            return true;
        }
    }
    return false;
}

static void twi0_stop(void)
{
    TWI0.MCTRLB = (uint8_t)TWI_MCMD_STOP_gc;
}

void twi0_init(uint32_t scl_hz)
{
    // MBAUD calculation (approx):
    //   MBAUD = (F_CPU / (2 * SCL)) - 5
    // This ignores rise time; fine for bring-up with typical pull-ups.
    uint32_t baud = (F_CPU / (2u * scl_hz));
    if (baud > 5u) {
        baud -= 5u;
    } else {
        baud = 1u;
    }
    if (baud > 255u) {
        baud = 255u;
    }

    TWI0.MBAUD = (uint8_t)baud;
    TWI0.MCTRLA = TWI_ENABLE_bm | TWI_SMEN_bm;
    TWI0.MSTATUS = (uint8_t)TWI_BUSSTATE_IDLE_gc;
}

bool twi0_address_only(uint8_t addr7)
{
    TWI0.MADDR = (uint8_t)(addr7 << 1);
    if (!twi0_wait(TWI_WIF_bm)) {
        twi0_stop();
        return false;
    }
    // RXACK=1 means NACK received; for address-only use (e.g. ATECC wake), allow NACK.
    twi0_stop();
    return true;
}

bool twi0_write(uint8_t addr7, const uint8_t *data, size_t len)
{
    TWI0.MADDR = (uint8_t)(addr7 << 1);
    if (!twi0_wait(TWI_WIF_bm)) {
        twi0_stop();
        return false;
    }
    if ((TWI0.MSTATUS & TWI_RXACK_bm) != 0) {
        twi0_stop();
        return false;
    }

    for (size_t i = 0; i < len; i++) {
        TWI0.MDATA = data[i];
        if (!twi0_wait(TWI_WIF_bm)) {
            twi0_stop();
            return false;
        }
        if ((TWI0.MSTATUS & TWI_RXACK_bm) != 0) {
            twi0_stop();
            return false;
        }
    }

    twi0_stop();
    return true;
}

bool twi0_read(uint8_t addr7, uint8_t *data, size_t len)
{
    if (len == 0) {
        return true;
    }

    TWI0.MADDR = (uint8_t)((addr7 << 1) | 0x01u);
    for (size_t i = 0; i < len; i++) {
        if (!twi0_wait(TWI_RIF_bm)) {
            twi0_stop();
            return false;
        }

        data[i] = TWI0.MDATA;

        if (i + 1 == len) {
            TWI0.MCTRLB = (uint8_t)TWI_ACKACT_NACK_gc | (uint8_t)TWI_MCMD_STOP_gc;
        } else {
            TWI0.MCTRLB = (uint8_t)TWI_MCMD_RECVTRANS_gc;
        }
    }

    return true;
}

