#include "mcu.h"
#include "spi.h"
#include "status_led.h"
#include "twi0.h"

#include "boot_contract.h"
#include "boot_eeprom.h"
#include "nvm_flash.h"
#include "slots.h"

#include "drivers/atecc608a_bl.h"
#include "drivers/w25q.h"

#include <util/delay.h>

static void fatal_blink(uint8_t code)
{
    for (;;) {
        status_led_blink_blocking(code, 6, 6);
        status_led_blink_blocking(1, 20, 20);
    }
}

static void app_jump(void)
{
    asm volatile("cli" ::: "memory");
    void (*app_reset)(void) = (void (*)(void))APP_START_WORD;
    app_reset();
}

static bool program_app_from_slot(uint8_t slot, const slot_header_t *hdr)
{
    uint8_t page[PROGMEM_PAGE_SIZE];
    uint32_t src = slot_image_base(slot);
    uint32_t dst = APP_START_ADDR;
    uint16_t remaining = (uint16_t)hdr->image_len;

    while (remaining) {
        uint16_t take = (remaining > PROGMEM_PAGE_SIZE) ? (uint16_t)PROGMEM_PAGE_SIZE : (uint16_t)remaining;

        if (!w25q_read(src, page, take)) {
            return false;
        }
        for (uint16_t i = take; i < PROGMEM_PAGE_SIZE; i++) {
            page[i] = 0xFF;
        }

        if (!nvm_flash_write_page(dst, page)) {
            return false;
        }


        src += take;
        dst += PROGMEM_PAGE_SIZE;
        remaining = (uint16_t)(remaining - take);
    }

    return true;
}

static bool program_app_from_slot_and_verify(uint8_t slot, const slot_header_t *hdr)
{
    uint8_t digest[32];
    uint8_t tail[63];
    uint8_t tail_len = 0;

    if (!atecc608a_bl_sha_start(ATECC608A_I2C_ADDR_DEFAULT)) {
        return false;
    }

    uint8_t page[PROGMEM_PAGE_SIZE];
    uint32_t src = slot_image_base(slot);
    uint32_t dst = APP_START_ADDR;
    uint16_t remaining = (uint16_t)hdr->image_len;
    while (remaining) {
        uint16_t take = (remaining > PROGMEM_PAGE_SIZE) ? (uint16_t)PROGMEM_PAGE_SIZE : (uint16_t)remaining;

        if (!w25q_read(src, page, take)) {
            return false;
        }

        // Feed SHA with exactly the image bytes (no padding).
        if (take >= 64) {
            if (!atecc608a_bl_sha_update64(ATECC608A_I2C_ADDR_DEFAULT, &page[0])) {
                return false;
            }
        } else {
            tail_len = (uint8_t)take;
            for (uint8_t i = 0; i < tail_len; i++) {
                tail[i] = page[i];
            }
        }
        if (take >= 128) {
            if (!atecc608a_bl_sha_update64(ATECC608A_I2C_ADDR_DEFAULT, &page[64])) {
                return false;
            }
        } else if (take > 64) {
            tail_len = (uint8_t)(take - 64);
            for (uint8_t i = 0; i < tail_len; i++) {
                tail[i] = page[64 + i];
            }
        }

        // Pad for flash programming.
        for (uint16_t i = take; i < PROGMEM_PAGE_SIZE; i++) {
            page[i] = 0xFF;
        }

        if (!nvm_flash_write_page(dst, page)) {
            return false;
        }

        // Verify readback via mapped flash.
        volatile const uint8_t *flash = (volatile const uint8_t *)(MAPPED_PROGMEM_START + dst);
        for (uint16_t i = 0; i < PROGMEM_PAGE_SIZE; i++) {
            if (flash[i] != page[i]) {
                return false;
            }
        }

        src += take;
        dst += PROGMEM_PAGE_SIZE;
        remaining = (uint16_t)(remaining - take);
    }

    if (!atecc608a_bl_sha_end(ATECC608A_I2C_ADDR_DEFAULT, tail, tail_len, digest)) {
        return false;
    }
    if (!atecc608a_bl_verify_stored_awake(ATECC608A_I2C_ADDR_DEFAULT, FW_ATECC_PUBKEY_SLOT, digest, hdr->sig_rs)) {
        (void)atecc608a_bl_sleep(ATECC608A_I2C_ADDR_DEFAULT);
        return false;
    }
    (void)atecc608a_bl_sleep(ATECC608A_I2C_ADDR_DEFAULT);
    return true;
}

int main(void)
{
    mcu_init();
    status_led_init();
    spi0_init();
    twi0_init(100000);

    w25q_pins_init();

    status_led_blink_blocking(1, 4, 4);

    if (!w25q_probe()) {
        fatal_blink(3);
    }

    boot_state_t st;
    boot_eeprom_load(&st);

    // If we are waiting for the app to confirm a previously applied update,
    // allow it to boot a few times. If it never confirms, roll back.
    if ((st.flags & BOOTF_WAIT_CONFIRM) != 0) {
        if (st.attempts >= 3) {
            slot_header_t hdr;
            if (!slot_read_header(st.confirmed_slot, &hdr)) {
                fatal_blink(7);
            }
            if (!program_app_from_slot(st.confirmed_slot, &hdr)) {
                fatal_blink(6);
            }

            st.pending_slot = 0xFF;
            st.flags = 0;
            st.attempts = 0;
            boot_eeprom_store(&st);
        } else {
            st.attempts++;
            boot_eeprom_store(&st);
        }

        app_jump();
    }

    // Apply a newly staged candidate slot if requested by the application.
    if ((st.flags & BOOTF_APPLY_PENDING) != 0 && st.pending_slot != 0xFF) {
        slot_header_t hdr;
        if (!slot_read_header(st.pending_slot, &hdr)) {
            st.pending_slot = 0xFF;
            st.flags &= (uint8_t)~BOOTF_APPLY_PENDING;
            boot_eeprom_store(&st);
            app_jump();
        }

        if (!program_app_from_slot_and_verify(st.pending_slot, &hdr)) {
            slot_header_t rh;
            if (!slot_read_header(st.confirmed_slot, &rh)) {
                fatal_blink(7);
            }
            if (!program_app_from_slot(st.confirmed_slot, &rh)) {
                fatal_blink(6);
            }

            st.pending_slot = 0xFF;
            st.flags &= (uint8_t)~BOOTF_APPLY_PENDING;
            st.attempts = 0;
            boot_eeprom_store(&st);
            app_jump();
        }

        // Mark as applied; app must confirm.
        st.flags &= (uint8_t)~BOOTF_APPLY_PENDING;
        st.flags |= BOOTF_WAIT_CONFIRM;
        st.attempts = 0;
        boot_eeprom_store(&st);

        app_jump();
    }

    // No pending work: boot the app.
    app_jump();
    for (;;) {
    }
}
