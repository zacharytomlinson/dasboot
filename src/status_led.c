#include "status_led.h"

#include "board.h"
#include "gpio.h"

#include <util/delay.h>

static inline void status_led_apply(bool on)
{
#if STAT_LED_ACTIVE_LOW
    if (on) {
        gpio_low(STAT_LED_PORT, STAT_LED_bm);
    } else {
        gpio_high(STAT_LED_PORT, STAT_LED_bm);
    }
#else
    if (on) {
        gpio_high(STAT_LED_PORT, STAT_LED_bm);
    } else {
        gpio_low(STAT_LED_PORT, STAT_LED_bm);
    }
#endif
}

void status_led_init(void)
{
    gpio_output(STAT_LED_PORT, STAT_LED_bm);
    status_led_apply(false);
}

void status_led_set(bool on)
{
    status_led_apply(on);
}

void status_led_toggle(void)
{
    STAT_LED_PORT->OUTTGL = STAT_LED_bm;
}

void status_led_blink_blocking(uint8_t times, uint8_t on_ticks, uint8_t off_ticks)
{
    for (uint8_t i = 0; i < times; i++) {
        status_led_set(true);
        for (uint8_t on = on_ticks; on > 0; on--) {
            _delay_ms(20);
        }
        status_led_set(false);
        for (uint8_t off = off_ticks; off > 0; off--) {
            _delay_ms(20);
        }
    }
}
