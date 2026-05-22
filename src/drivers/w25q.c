#include "w25q.h"

#include "../board.h"
#include "../gpio.h"
#include "../spi.h"

void w25q_pins_init(void)
{
    PORTD.OUTSET = W25Q_RESET_bm | W25Q_CS_bm | W25Q_WP_bm;
    PORTD.DIRSET = W25Q_RESET_bm | W25Q_CS_bm | W25Q_WP_bm;
}

static inline void w25q_select(void)
{
    gpio_low(W25Q_CS_PORT, W25Q_CS_bm);
}

static inline void w25q_deselect(void)
{
    gpio_high(W25Q_CS_PORT, W25Q_CS_bm);
}

w25q_jedec_id_t w25q_read_jedec_id(void)
{
    // Command 0x9F: Read JEDEC ID (3 bytes).
    uint8_t cmd = 0x9F;
    uint8_t rx[3] = {0};

    w25q_select();
    spi0_transfer_buf(&cmd, 0, 1);
    spi0_transfer_buf(0, rx, 3);
    w25q_deselect();

    return (w25q_jedec_id_t){
            .manufacturer_id = rx[0],
            .memory_type = rx[1],
            .capacity = rx[2],
    };
}

bool w25q_probe(void)
{
    w25q_jedec_id_t id = w25q_read_jedec_id();
    // Manufacturer ID is 0xEF for Winbond; allow any non-0x00/0xFF while bring-up.
    if (id.manufacturer_id == 0x00 || id.manufacturer_id == 0xFF) {
        return false;
    }
    return true;
}

bool w25q_read(uint32_t addr, uint8_t *buf, uint16_t len)
{
    // Command 0x03: Read Data (3-byte address).
    uint8_t hdr[4] = {
            0x03,
            (uint8_t)(addr >> 16),
            (uint8_t)(addr >> 8),
            (uint8_t)(addr),
    };

    w25q_select();
    spi0_transfer_buf(hdr, 0, 4);
    spi0_transfer_buf(0, buf, len);
    w25q_deselect();
    return true;
}
