#include "mcu.h"
#include "status_led.h"
#include "twi0.h"

#include "drivers/atecc608a.h"

#include <util/delay.h>

// Safety: this tool permanently locks the ATECC if enabled.
// To actually run provisioning, set this to 1 at compile time:
//   -DATECC_PROVISION_ENABLE=1
#ifndef ATECC_PROVISION_ENABLE
#define ATECC_PROVISION_ENABLE 0
#endif

static void blink_forever(uint8_t code)
{
    for (;;) {
        status_led_blink_blocking(code, 120, 120);
        status_led_blink_blocking(1, 400, 400);
    }
}

int main(void)
{
    mcu_init();
    status_led_init();
    twi0_init(100000);

    atecc608a_t atecc = {.i2c_addr = ATECC608A_I2C_ADDR_DEFAULT};
    if (!atecc608a_info_revision(&atecc)) {
        blink_forever(2);
    }

    if (atecc608a_is_configured_default(&atecc)) {
        // Already provisioned.
        blink_forever(1);
    }

#if ATECC_PROVISION_ENABLE
    // Mirrors the legacy Arduino flow: writes minimal config, locks config, generates slot0 keypair, locks data+slot0.
    if (!atecc608a_configure_default(&atecc)) {
        blink_forever(5);
    }
    blink_forever(6);
#else
    // Not provisioned yet, and provisioning is disabled in this build.
    blink_forever(4);
#endif
}

