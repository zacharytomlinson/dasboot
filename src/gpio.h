#pragma once

#include <avr/io.h>
#include <stdint.h>

static inline void gpio_output(volatile PORT_t *port, uint8_t pin_bm)
{
    port->DIRSET = pin_bm;
}

static inline void gpio_input(volatile PORT_t *port, uint8_t pin_bm)
{
    port->DIRCLR = pin_bm;
}

static inline void gpio_high(volatile PORT_t *port, uint8_t pin_bm)
{
    port->OUTSET = pin_bm;
}

static inline void gpio_low(volatile PORT_t *port, uint8_t pin_bm)
{
    port->OUTCLR = pin_bm;
}

static inline uint8_t gpio_pin_index_from_bm(uint8_t pin_bm)
{
    for (uint8_t i = 0; i < 8; i++) {
        if (pin_bm == (uint8_t)(1u << i)) {
            return i;
        }
    }
    return 0;
}

static inline volatile uint8_t *gpio_pinctrl_reg(volatile PORT_t *port, uint8_t pin_index)
{
    return ((volatile uint8_t *)&port->PIN0CTRL) + pin_index;
}

