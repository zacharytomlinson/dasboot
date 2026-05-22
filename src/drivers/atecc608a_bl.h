#pragma once

#include <stdbool.h>
#include <stdint.h>

#define ATECC608A_I2C_ADDR_DEFAULT 0x60

bool atecc608a_bl_wake(uint8_t i2c_addr);
bool atecc608a_bl_sleep(uint8_t i2c_addr);
bool atecc608a_bl_idle(uint8_t i2c_addr);

bool atecc608a_bl_sha_start(uint8_t i2c_addr);
bool atecc608a_bl_sha_update64(uint8_t i2c_addr, const uint8_t data[64]);
bool atecc608a_bl_sha_end(uint8_t i2c_addr, const uint8_t *data, uint8_t len, uint8_t digest[32]);

bool atecc608a_bl_verify_stored_awake(uint8_t i2c_addr, uint8_t key_slot, const uint8_t digest[32],
                                      const uint8_t sig_rs[64]);
