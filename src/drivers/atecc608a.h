#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t i2c_addr; // 7-bit address
    uint8_t rx_buf[128];
    uint8_t sha_rem[64];
    uint8_t sha_rem_len;
} atecc608a_t;

#define ATECC608A_I2C_ADDR_DEFAULT 0x60

bool atecc608a_wake(atecc608a_t *dev);
bool atecc608a_idle(atecc608a_t *dev);
bool atecc608a_sleep(atecc608a_t *dev);

bool atecc608a_info_revision(atecc608a_t *dev);

// Computes SHA-256 on arbitrary-length input (implemented via ATECC SHA command).
bool atecc608a_sha256(atecc608a_t *dev, const uint8_t *plain, size_t len, uint8_t hash[32]);

// Incremental SHA-256 (lower RAM usage when hashing flash contents).
bool atecc608a_sha256_start(atecc608a_t *dev);
bool atecc608a_sha256_update(atecc608a_t *dev, const uint8_t *chunk, size_t len);
bool atecc608a_sha256_finish(atecc608a_t *dev, uint8_t hash[32]);

// Verifies a signature against a 32-byte message and an external 64-byte public key.
bool atecc608a_verify_external(atecc608a_t *dev, const uint8_t message[32], const uint8_t signature[64],
                               const uint8_t public_key[64]);

// Verifies a signature against a 32-byte message using a public key stored in an ATECC slot.
bool atecc608a_verify_stored(atecc608a_t *dev, uint16_t slot, const uint8_t message[32], const uint8_t signature[64]);

typedef struct {
    bool config_locked;
    bool data_otp_locked;
    uint8_t slots_lock0; // slots 0..7, 1=unlocked
    uint8_t slots_lock1; // slots 8..15, 1=unlocked
} atecc608a_lock_status_t;

// Reads lock status bytes from config zone (does not modify the device).
bool atecc608a_get_lock_status(atecc608a_t *dev, atecc608a_lock_status_t *out);

bool atecc608a_is_slot_locked(const atecc608a_lock_status_t *st, uint8_t slot);

// Optional provisioning helpers (SparkFun-style default config + permanent lock).
bool atecc608a_is_configured_default(atecc608a_t *dev);
bool atecc608a_configure_default(atecc608a_t *dev);

// Bootloader-oriented check: config+data locked and a public key slot locked.
bool atecc608a_is_configured_for_pubkey_slot(atecc608a_t *dev, uint8_t pubkey_slot);
