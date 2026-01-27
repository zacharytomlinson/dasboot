#include "nvm_flash.h"

#include <avr/cpufunc.h>
#include <avr/io.h>

static inline void nvm_wait_flash(void)
{
    while ((NVMCTRL.STATUS & NVMCTRL_FBUSY_bm) != 0) {
    }
}

static inline void nvm_exec(uint8_t cmd)
{
    nvm_wait_flash();
    ccp_write_io((uint8_t *)&NVMCTRL.CTRLA, cmd);
    nvm_wait_flash();
}

bool nvm_flash_write_page(uint16_t flash_addr, const uint8_t page[PROGMEM_PAGE_SIZE])
{
    // Clear page buffer.
    nvm_exec(NVMCTRL_CMD_PAGEBUFCLR_gc);

    // Load page buffer via DATA/ADDR writes (16-bit words).
    for (uint16_t i = 0; i < PROGMEM_PAGE_SIZE; i += 2) {
        uint16_t word = (uint16_t)page[i] | ((uint16_t)page[i + 1] << 8);
        NVMCTRL.ADDR = (uint16_t)(flash_addr + i);
        NVMCTRL.DATA = word;
    }

    // Erase + write the page at flash_addr.
    NVMCTRL.ADDR = flash_addr;
    nvm_exec(NVMCTRL_CMD_PAGEERASEWRITE_gc);

    if ((NVMCTRL.STATUS & NVMCTRL_WRERROR_bm) != 0) {
        return false;
    }
    return true;
}

