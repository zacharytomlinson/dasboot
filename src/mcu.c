#include "mcu.h"

#include "board.h"

#include <avr/cpufunc.h>
#include <avr/interrupt.h>

static void clock_init(void)
{
    // Switch main clock to internal 16/20 MHz oscillator and disable prescaler.
    ccp_write_io((uint8_t *)&CLKCTRL.MCLKCTRLA, CLKCTRL_CLKSEL_OSC20M_gc);
    ccp_write_io((uint8_t *)&CLKCTRL.MCLKCTRLB, 0);
    while ((CLKCTRL.MCLKSTATUS & CLKCTRL_OSC20MS_bm) == 0) {
    }
}

static void watchdog_disable(void)
{
    // If WDT is enabled by fuse, disable it early.
    while ((WDT.STATUS & WDT_SYNCBUSY_bm) != 0) {
    }
    ccp_write_io((uint8_t *)&WDT.CTRLA, 0);
    while ((WDT.STATUS & WDT_SYNCBUSY_bm) != 0) {
    }
}

static void portmux_init(void)
{
    PORTMUX.TWISPIROUTEA =
            (PORTMUX.TWISPIROUTEA & ~(PORTMUX_SPI0_gm | PORTMUX_TWI0_gm)) |
            ((uint8_t)SPI0_PORTMUX_ROUTE_SEL << PORTMUX_SPI0_gp) |
            ((uint8_t)TWI0_PORTMUX_ROUTE_SEL << PORTMUX_TWI0_gp);
    PORTMUX.USARTROUTEA =
            (PORTMUX.USARTROUTEA & ~PORTMUX_USART0_gm) | ((uint8_t)USART0_PORTMUX_ROUTE_SEL << PORTMUX_USART0_gp);
}

void mcu_init(void)
{
    cli();
    watchdog_disable();
    clock_init();
    portmux_init();
}
