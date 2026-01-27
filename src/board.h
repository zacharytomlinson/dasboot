#pragma once

#include <avr/io.h>

// Board/pin mapping defaults.
// Adjust these to match your schematic and PORTMUX routing choices.

// SPI0 default route: PA[7:4]
// Current board wiring:
//   PA4 = MOSI, PA5 = MISO, PA6 = SCK, PA7 = Ethernet CS
//
// PORTMUX selection values:
//   0 = DEFAULT, 1 = ALT1, 2 = ALT2, 3 = NONE
#define SPI0_PORTMUX_ROUTE_SEL 0

// W25Qxx control pins
#define W25Q_CS_PORT (&PORTD)
#define W25Q_CS_bm PIN2_bm

// Status LED ("STAT")
#define STAT_LED_PORT (&PORTD)
#define STAT_LED_bm PIN0_bm
// Set to 1 if LED is wired to VCC and sinks current (active-low).
#define STAT_LED_ACTIVE_LOW 0

// Optional: debug UART (USART0 default route: PA[3:0])
#define USART0_PORTMUX_ROUTE_SEL 0

#define SPI0_MOSI_bm PIN4_bm
#define SPI0_MISO_bm PIN5_bm
#define SPI0_SCK_bm PIN6_bm
#define SPI0_SS_bm PIN7_bm

// TWI0 PORTMUX selection values:
//   0 = DEFAULT (PA3:2), 1 = ALT1 (PA3:2), 2 = ALT2 (PC3:2), 3 = NONE
// Pick a route that matches your board wiring.
#define TWI0_PORTMUX_ROUTE_SEL 2
