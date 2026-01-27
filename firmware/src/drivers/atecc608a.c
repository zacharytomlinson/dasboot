#include "atecc608a.h"

#include "../twi0.h"

#include <string.h>
#include <util/delay.h>

enum {
    WORD_ADDR_COMMAND = 0x03,
    WORD_ADDR_IDLE = 0x02,
    WORD_ADDR_SLEEP = 0x01,
};

enum {
    OPCODE_INFO = 0x30,
    OPCODE_LOCK = 0x17,
    OPCODE_GENKEY = 0x40,
    OPCODE_READ = 0x02,
    OPCODE_WRITE = 0x12,
    OPCODE_NONCE = 0x16,
    OPCODE_VERIFY = 0x45,
    OPCODE_SHA = 0x47,
};

enum {
    SHA_START = 0x00,
    SHA_UPDATE = 0x01,
    SHA_END = 0x02,
    SHA_BLOCK_SIZE = 64,
};

enum {
    NONCE_MODE_PASSTHROUGH = 0x03,
    VERIFY_MODE_EXTERNAL = 0x02,
    VERIFY_MODE_STORED = 0x00,
    VERIFY_PARAM2_KEYTYPE_ECC = 0x0004,
};

enum {
    ZONE_CONFIG = 0x00,
};

enum {
    LOCK_MODE_ZONE_CONFIG = 0x80,
    LOCK_MODE_ZONE_DATA_AND_OTP = 0x81,
    LOCK_MODE_SLOT0 = 0x82,
};

enum {
    GENKEY_MODE_NEW_PRIVATE = 0x04,
};

enum {
    CONFIG_ZONE_OTP_LOCK = 86,
    CONFIG_ZONE_LOCK_STATUS = 87,
    CONFIG_ZONE_SLOTS_LOCK0 = 88,
    CONFIG_ZONE_SLOTS_LOCK1 = 89,
};

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

static bool atecc_check_response_crc(const uint8_t *buf, uint8_t len)
{
    if (len < 3) {
        return false;
    }
    uint8_t crc[2] = {0};
    atca_calculate_crc((uint8_t)(len - 2u), buf, crc);
    return (buf[len - 2u] == crc[0]) && (buf[len - 1u] == crc[1]);
}

static bool atecc_receive_fixed(atecc608a_t *dev, uint8_t expected_len, uint16_t delay_ms)
{
    // Poll reads until we get something plausible (not all 0xFF/0x00), or timeout.
    for (uint16_t t = 0; t < delay_ms; t++) {
        if (twi0_read(dev->i2c_addr, dev->rx_buf, expected_len)) {
            uint8_t count = dev->rx_buf[0];
            if (count == expected_len && atecc_check_response_crc(dev->rx_buf, expected_len)) {
                return true;
            }
        }
        _delay_ms(1);
    }
    return false;
}

static bool atecc_send_command(atecc608a_t *dev, uint8_t opcode, uint8_t param1, uint16_t param2, const uint8_t *data,
                               uint8_t data_len)
{
    // Packet: [WORD_ADDR][COUNT][OPCODE][PARAM1][PARAM2_L][PARAM2_H][DATA...][CRC0][CRC1]
    // COUNT excludes WORD_ADDR but includes COUNT itself + CRC.
    enum { ATECC_MAX_DATA_LEN = 128 };
    enum { ATECC_TX_MAX = 1u + 1u + 1u + 1u + 2u + ATECC_MAX_DATA_LEN + 2u }; // 136 bytes
    uint8_t tx[ATECC_TX_MAX];
    uint8_t payload_len = (uint8_t)(1u /*count*/ + 1u /*opcode*/ + 1u /*param1*/ + 2u /*param2*/ + data_len + 2u);
    uint8_t total_len = (uint8_t)(1u /*word*/ + payload_len);

    if (total_len > sizeof(tx)) {
        return false;
    }

    tx[0] = WORD_ADDR_COMMAND;
    tx[1] = payload_len;
    tx[2] = opcode;
    tx[3] = param1;
    tx[4] = (uint8_t)(param2 & 0xFFu);
    tx[5] = (uint8_t)(param2 >> 8);
    if (data_len && data) {
        memcpy(&tx[6], data, data_len);
    }

    uint8_t crc[2] = {0};
    atca_calculate_crc((uint8_t)(payload_len - 2u), &tx[1], crc);
    tx[total_len - 2u] = crc[0];
    tx[total_len - 1u] = crc[1];

    return twi0_write(dev->i2c_addr, tx, total_len);
}

static bool atecc_read_zone(atecc608a_t *dev, uint8_t zone, uint16_t address, uint8_t length, uint8_t *out)
{
    // For ATECC, bit7 of zone selects 32-byte (1) vs 4-byte (0) operation.
    if (length == 32) {
        zone |= 0x80;
    } else if (length == 4) {
        zone &= (uint8_t)~0x80;
    } else {
        return false;
    }

    if (!atecc_send_command(dev, OPCODE_READ, zone, address, 0, 0)) {
        return false;
    }
    if (!atecc_receive_fixed(dev, (uint8_t)(1u + length + 2u), 10)) {
        return false;
    }
    if (out) {
        memcpy(out, &dev->rx_buf[1], length);
    }
    return true;
}

static bool atecc_write_zone(atecc608a_t *dev, uint8_t zone, uint16_t address, const uint8_t *data, uint8_t length)
{
    if (length == 32) {
        zone |= 0x80;
    } else if (length == 4) {
        zone &= (uint8_t)~0x80;
    } else {
        return false;
    }

    if (!atecc_send_command(dev, OPCODE_WRITE, zone, address, data, length)) {
        return false;
    }
    if (!atecc_receive_fixed(dev, 1u + 1u + 2u, 40)) {
        return false;
    }
    return dev->rx_buf[1] == 0x00;
}

static bool atecc_lock(atecc608a_t *dev, uint8_t mode)
{
    if (!atecc_send_command(dev, OPCODE_LOCK, mode, 0x0000, 0, 0)) {
        return false;
    }
    if (!atecc_receive_fixed(dev, 1u + 1u + 2u, 60)) {
        return false;
    }
    return dev->rx_buf[1] == 0x00;
}

bool atecc608a_wake(atecc608a_t *dev)
{
    // Create a wake condition by addressing 0x00 at 100 kHz (device will NACK).
    (void)twi0_address_only(0x00);
    _delay_us(1500);

    if (!twi0_read(dev->i2c_addr, dev->rx_buf, 4)) {
        return false;
    }
    if (dev->rx_buf[0] != 0x04) {
        return false;
    }
    if (!atecc_check_response_crc(dev->rx_buf, 4)) {
        return false;
    }
    return dev->rx_buf[1] == 0x11;
}

bool atecc608a_idle(atecc608a_t *dev)
{
    uint8_t b = WORD_ADDR_IDLE;
    return twi0_write(dev->i2c_addr, &b, 1);
}

bool atecc608a_sleep(atecc608a_t *dev)
{
    (void)atecc608a_idle(dev);
    uint8_t b = WORD_ADDR_SLEEP;
    return twi0_write(dev->i2c_addr, &b, 1);
}

bool atecc608a_info_revision(atecc608a_t *dev)
{
    if (!atecc608a_wake(dev)) {
        return false;
    }
    if (!atecc_send_command(dev, OPCODE_INFO, 0x00, 0x0000, 0, 0)) {
        return false;
    }
    if (!atecc_receive_fixed(dev, 1u + 4u + 2u, 10)) {
        return false;
    }
    // Expected revision signature byte is typically 0x50 in the 3rd data byte.
    // Response layout: [count][b0][b1][b2][b3][crc0][crc1]
    return dev->rx_buf[3] == 0x50;
}

static bool atecc_load_tempkey(atecc608a_t *dev, const uint8_t message[32])
{
    if (!atecc_send_command(dev, OPCODE_NONCE, NONCE_MODE_PASSTHROUGH, 0x0000, message, 32)) {
        return false;
    }
    if (!atecc_receive_fixed(dev, 1u + 1u + 2u, 10)) {
        return false;
    }
    return dev->rx_buf[1] == 0x00;
}

bool atecc608a_verify_external(atecc608a_t *dev, const uint8_t message[32], const uint8_t signature[64],
                               const uint8_t public_key[64])
{
    if (!atecc608a_wake(dev)) {
        return false;
    }
    if (!atecc_load_tempkey(dev, message)) {
        return false;
    }

    // VERIFY external expects signature (64) + public key (64) as a single data payload.
    enum { DATA_LEN = 128 };
    enum { PAYLOAD_LEN = 1u + 1u + 1u + 2u + DATA_LEN + 2u };
    enum { TOTAL_LEN = 1u + PAYLOAD_LEN };
    uint8_t tx[TOTAL_LEN];

    tx[0] = WORD_ADDR_COMMAND;
    tx[1] = PAYLOAD_LEN;
    tx[2] = OPCODE_VERIFY;
    tx[3] = VERIFY_MODE_EXTERNAL;
    tx[4] = (uint8_t)(VERIFY_PARAM2_KEYTYPE_ECC & 0xFFu);
    tx[5] = (uint8_t)(VERIFY_PARAM2_KEYTYPE_ECC >> 8);
    memcpy(&tx[6], signature, 64);
    memcpy(&tx[6 + 64], public_key, 64);

    uint8_t crc[2] = {0};
    atca_calculate_crc((uint8_t)(PAYLOAD_LEN - 2u), &tx[1], crc);
    tx[TOTAL_LEN - 2u] = crc[0];
    tx[TOTAL_LEN - 1u] = crc[1];

    if (!twi0_write(dev->i2c_addr, tx, TOTAL_LEN)) {
        return false;
    }

    if (!atecc_receive_fixed(dev, 1u + 1u + 2u, 70)) {
        return false;
    }
    return dev->rx_buf[1] == 0x00;
}

bool atecc608a_verify_stored(atecc608a_t *dev, uint16_t slot, const uint8_t message[32], const uint8_t signature[64])
{
    if (!atecc608a_wake(dev)) {
        return false;
    }
    if (!atecc_load_tempkey(dev, message)) {
        return false;
    }
    if (!atecc_send_command(dev, OPCODE_VERIFY, VERIFY_MODE_STORED, slot, signature, 64)) {
        return false;
    }
    if (!atecc_receive_fixed(dev, 1u + 1u + 2u, 70)) {
        return false;
    }
    return dev->rx_buf[1] == 0x00;
}

bool atecc608a_sha256_start(atecc608a_t *dev)
{
    if (!atecc608a_wake(dev)) {
        return false;
    }
    dev->sha_rem_len = 0;
    if (!atecc_send_command(dev, OPCODE_SHA, SHA_START, 0x0000, 0, 0)) {
        return false;
    }
    // After START, device returns a 1-byte status response.
    if (!atecc_receive_fixed(dev, 1u + 1u + 2u, 20)) {
        return false;
    }
    return dev->rx_buf[1] == 0x00;
}

bool atecc608a_sha256_update(atecc608a_t *dev, const uint8_t *chunk, size_t len)
{
    // Buffer up to 64 bytes; send full 64-byte blocks as SHA_UPDATE.
    while (len) {
        uint8_t take = (uint8_t)(len > (size_t)(64u - dev->sha_rem_len) ? (size_t)(64u - dev->sha_rem_len) : len);
        memcpy(&dev->sha_rem[dev->sha_rem_len], chunk, take);
        dev->sha_rem_len = (uint8_t)(dev->sha_rem_len + take);
        chunk += take;
        len -= take;

        if (dev->sha_rem_len == 64) {
            if (!atecc_send_command(dev, OPCODE_SHA, SHA_UPDATE, 64, dev->sha_rem, 64)) {
                return false;
            }
            if (!atecc_receive_fixed(dev, 1u + 1u + 2u, 30)) {
                return false;
            }
            if (dev->rx_buf[1] != 0x00) {
                return false;
            }
            dev->sha_rem_len = 0;
        }
    }
    return true;
}

bool atecc608a_sha256_finish(atecc608a_t *dev, uint8_t hash[32])
{
    // SHA_END can accept up to 63 bytes; if remaining is 64/0 boundary, remaining is 0.
    if (dev->sha_rem_len >= 64) {
        return false;
    }

    if (!atecc_send_command(dev, OPCODE_SHA, SHA_END, dev->sha_rem_len, dev->sha_rem, dev->sha_rem_len)) {
        return false;
    }

    // After END, device returns the 32-byte digest.
    if (!atecc_receive_fixed(dev, 1u + 32u + 2u, 60)) {
        return false;
    }
    memcpy(hash, &dev->rx_buf[1], 32);
    dev->sha_rem_len = 0;
    return true;
}

bool atecc608a_sha256(atecc608a_t *dev, const uint8_t *plain, size_t len, uint8_t hash[32])
{
    if (!atecc608a_sha256_start(dev)) {
        return false;
    }
    if (!atecc608a_sha256_update(dev, plain, len)) {
        return false;
    }
    return atecc608a_sha256_finish(dev, hash);
}

bool atecc608a_get_lock_status(atecc608a_t *dev, atecc608a_lock_status_t *out)
{
    if (!out) {
        return false;
    }

    // Lock/status bytes are in config zone around offsets 86-89, which are inside block 2 (offset 64..95).
    // Config block addresses for 32-byte reads are in "words" (offset/4). Block2 starts at 64/4 = 0x10.
    uint8_t block2[32];

    if (!atecc608a_wake(dev)) {
        return false;
    }
    if (!atecc_read_zone(dev, ZONE_CONFIG, 0x0010, 32, block2)) {
        return false;
    }

    uint8_t otp_lock = block2[CONFIG_ZONE_OTP_LOCK - 64];
    uint8_t cfg_lock = block2[CONFIG_ZONE_LOCK_STATUS - 64];
    uint8_t slots_lock0 = block2[CONFIG_ZONE_SLOTS_LOCK0 - 64];
    uint8_t slots_lock1 = block2[CONFIG_ZONE_SLOTS_LOCK1 - 64];

    out->config_locked = (cfg_lock == 0x00);
    out->data_otp_locked = (otp_lock == 0x00);
    out->slots_lock0 = slots_lock0;
    out->slots_lock1 = slots_lock1;
    return true;
}

bool atecc608a_is_slot_locked(const atecc608a_lock_status_t *st, uint8_t slot)
{
    if (!st) {
        return false;
    }
    if (slot < 8) {
        return ((st->slots_lock0 & (uint8_t)(1u << slot)) == 0);
    }
    if (slot < 16) {
        return ((st->slots_lock1 & (uint8_t)(1u << (slot - 8u))) == 0);
    }
    return false;
}

bool atecc608a_is_configured_default(atecc608a_t *dev)
{
    atecc608a_lock_status_t st;
    if (!atecc608a_get_lock_status(dev, &st)) {
        return false;
    }
    return st.config_locked && st.data_otp_locked && atecc608a_is_slot_locked(&st, 0);
}

bool atecc608a_is_configured_for_pubkey_slot(atecc608a_t *dev, uint8_t pubkey_slot)
{
    atecc608a_lock_status_t st;
    if (!atecc608a_get_lock_status(dev, &st)) {
        return false;
    }
    return st.config_locked && st.data_otp_locked && atecc608a_is_slot_locked(&st, pubkey_slot);
}

bool atecc608a_configure_default(atecc608a_t *dev)
{
    // Mirrors the bundled `HardwareRng::configure_chip()` behavior in `atecca.h`:
    // - write SparkFun-style config for slot 0/1
    // - lock config
    // - generate a new keypair in slot 0
    // - lock data/otp and slot 0
    //
    // WARNING: Locking is permanent. Only call this on blank/unprovisioned devices.
    static const uint8_t key_config_slot0_1[4] = {0x33, 0x00, 0x33, 0x00};  // config zone offset 96
    static const uint8_t slot_config_slot0_1[4] = {0x83, 0x20, 0x83, 0x20}; // config zone offset 20

    if (!atecc608a_wake(dev)) {
        return false;
    }

    if (!atecc_write_zone(dev, ZONE_CONFIG, (uint16_t)(96u / 4u), key_config_slot0_1, 4)) {
        return false;
    }
    if (!atecc_write_zone(dev, ZONE_CONFIG, (uint16_t)(20u / 4u), slot_config_slot0_1, 4)) {
        return false;
    }

    if (!atecc_lock(dev, LOCK_MODE_ZONE_CONFIG)) {
        return false;
    }

    if (!atecc_send_command(dev, OPCODE_GENKEY, GENKEY_MODE_NEW_PRIVATE, 0x0000, 0, 0)) {
        return false;
    }
    if (!atecc_receive_fixed(dev, 1u + 64u + 2u, 150)) {
        return false;
    }

    if (!atecc_lock(dev, LOCK_MODE_ZONE_DATA_AND_OTP)) {
        return false;
    }
    if (!atecc_lock(dev, LOCK_MODE_SLOT0)) {
        return false;
    }

    return true;
}
