#pragma once

#include <stdbool.h>
#include <stdint.h>

void status_led_init(void);
void status_led_set(bool on);
void status_led_toggle(void);
void status_led_blink_blocking(uint8_t times, uint16_t on_ms, uint16_t off_ms);

