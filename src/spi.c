#include "spi.h"

#include "board.h"
#include "gpio.h"

void spi0_init(void)
{
    // SPI0 pins on PA[7:4] for this board.
    // Keep SS as output to remain master.
    gpio_output(&PORTA, SPI0_SS_bm);
    gpio_output(&PORTA, SPI0_MOSI_bm);
    gpio_output(&PORTA, SPI0_SCK_bm);
    gpio_input(&PORTA, SPI0_MISO_bm);

    gpio_high(&PORTA, SPI0_SS_bm);

    // Default CS pins high (deselected)
    gpio_output(W25Q_CS_PORT, W25Q_CS_bm);
    gpio_high(W25Q_CS_PORT, W25Q_CS_bm);

    // Enable SPI in master mode, mode 0, prescaler /4 (adjust as needed).
    SPI0.CTRLA = SPI_ENABLE_bm | SPI_MASTER_bm | SPI_PRESC_DIV4_gc;
    SPI0.CTRLB = SPI_SSD_bm;
}

uint8_t spi0_transfer(uint8_t byte)
{
    SPI0.DATA = byte;
    while ((SPI0.INTFLAGS & SPI_IF_bm) == 0) {
    }
    return SPI0.DATA;
}

void spi0_transfer_buf(const uint8_t *tx, uint8_t *rx, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        uint8_t out = tx ? tx[i] : 0xFF;
        uint8_t in = spi0_transfer(out);
        if (rx) {
            rx[i] = in;
        }
    }
}
