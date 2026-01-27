#include "atecc608a_bl.h"

#include "../twi0.h"

#include <util/delay.h>

enum {
    WORD_ADDR_COMMAND = 0x03,
    WORD_ADDR_SLEEP = 0x01,
};

enum { OPCODE_NONCE = 0x16, OPCODE_VERIFY = 0x45, OPCODE_SHA = 0x47 };

enum { NONCE_MODE_PASSTHROUGH = 0x03, VERIFY_MODE_STORED = 0x00, SHA_START = 0x00, SHA_UPDATE = 0x01, SHA_END = 0x02 };

static void atca_calculate_crc(uint8_t length, const uint8_t *data, uint8_t crc[2])
{
    uint16_t crc_register = 0;
    uint16_t polynom = 0x8005;
    for (uint8_t counter = 0; counter < length; counter++) {
        for (uint8_t shift_register = 0x01; shift_register > 0x00; shift_register <<= 1) {
            uint8_t data_bit = (data[counter] & shift_register) ? 1 : 0;
            uint8_t crc_bit = (uint8_t)(crc_register >> 15);
            crc_register <<= 1;
            if (data_bit != crc_bit) {
                crc_register ^= polynom;
            }
        }
    }
    crc[0] = (uint8_t)(crc_register & 0x00FF);
    crc[1] = (uint8_t)(crc_register >> 8);
}

static bool atecc_read(uint8_t i2c_addr, uint8_t *buf, uint8_t len)
{
    if (!twi0_read(i2c_addr, buf, len)) {
        return false;
    }
    if (buf[0] != len) {
        return false;
    }
    uint8_t crc[2];
    atca_calculate_crc((uint8_t)(len - 2u), buf, crc);
    return (buf[len - 2u] == crc[0]) && (buf[len - 1u] == crc[1]);
}

static bool atecc_cmd(uint8_t i2c_addr, uint8_t opcode, uint8_t param1, uint16_t param2, const uint8_t *data,
                      uint8_t data_len)
{
    // [word][count][opcode][p1][p2l][p2h][data...][crc0][crc1]
    uint8_t tx[1u + 1u + 1u + 1u + 2u + 64u + 2u];

    uint8_t count = (uint8_t)(1u + 1u + 1u + 2u + data_len + 2u);
    uint8_t total = (uint8_t)(1u + count);

    tx[0] = WORD_ADDR_COMMAND;
    tx[1] = count;
    tx[2] = opcode;
    tx[3] = param1;
    tx[4] = (uint8_t)(param2 & 0xFFu);
    tx[5] = (uint8_t)(param2 >> 8);
    for (uint8_t i = 0; i < data_len; i++) {
        tx[6 + i] = data[i];
    }

    uint8_t crc[2];
    atca_calculate_crc((uint8_t)(count - 2u), &tx[1], crc);
    tx[total - 2u] = crc[0];
    tx[total - 1u] = crc[1];

    return twi0_write(i2c_addr, tx, total);
}

bool atecc608a_bl_wake(uint8_t i2c_addr)
{
    (void)twi0_address_only(0x00);
    _delay_us(1500);
    uint8_t resp[4];
    if (!atecc_read(i2c_addr, resp, 4)) {
        return false;
    }
    return resp[1] == 0x11;
}

bool atecc608a_bl_sleep(uint8_t i2c_addr)
{
    uint8_t b = WORD_ADDR_SLEEP;
    return twi0_write(i2c_addr, &b, 1);
}

static bool atecc_status_ok(uint8_t i2c_addr, uint8_t delay_ms)
{
    while (delay_ms--) {
        _delay_ms(1);
    }
    uint8_t resp[4];
    if (!atecc_read(i2c_addr, resp, 4)) {
        return false;
    }
    return resp[1] == 0x00;
}

bool atecc608a_bl_sha_start(uint8_t i2c_addr)
{
    if (!atecc608a_bl_wake(i2c_addr)) {
        return false;
    }

    if (!atecc_cmd(i2c_addr, OPCODE_SHA, SHA_START, 0, 0, 0)) {
        return false;
    }
    return atecc_status_ok(i2c_addr, 9);
}

bool atecc608a_bl_sha_update64(uint8_t i2c_addr, const uint8_t data[64])
{
    if (!atecc_cmd(i2c_addr, OPCODE_SHA, SHA_UPDATE, 64, data, 64)) {
        return false;
    }
    return atecc_status_ok(i2c_addr, 9);
}

bool atecc608a_bl_sha_end(uint8_t i2c_addr, const uint8_t *data, uint8_t len, uint8_t digest[32])
{
    if (len > 63) {
        return false;
    }
    if (!atecc_cmd(i2c_addr, OPCODE_SHA, SHA_END, len, data, len)) {
        return false;
    }

    _delay_ms(9);
    uint8_t resp[35];
    if (!atecc_read(i2c_addr, resp, 35)) {
        return false;
    }
    for (uint8_t i = 0; i < 32; i++) {
        digest[i] = resp[1 + i];
    }
    return true;
}

bool atecc608a_bl_verify_stored_awake(uint8_t i2c_addr, uint8_t key_slot, const uint8_t digest[32],
                                      const uint8_t sig_rs[64])
{
    // Load TempKey with digest using NONCE passthrough (32 bytes).
    if (!atecc_cmd(i2c_addr, OPCODE_NONCE, NONCE_MODE_PASSTHROUGH, 0, digest, 32)) {
        return false;
    }
    if (!atecc_status_ok(i2c_addr, 7)) {
        return false;
    }

    // VERIFY stored: data is signature (64), param2 is key slot.
    if (!atecc_cmd(i2c_addr, OPCODE_VERIFY, VERIFY_MODE_STORED, key_slot, sig_rs, 64)) {
        return false;
    }
    if (!atecc_status_ok(i2c_addr, 60)) {
        return false;
    }
    return true;
}
