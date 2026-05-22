#include "nvm_flash.h"

#include <avr/cpufunc.h>
#include <avr/io.h>
#include <avr/xmega.h>

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

bool nvm_flash_write_page(uint32_t flash_addr, const uint8_t page[PROGMEM_PAGE_SIZE])
{
    nvm_exec(NVMCTRL_CMD_PAGEBUFCLR_gc);

    volatile uint8_t *dst = (volatile uint8_t *)(MAPPED_PROGMEM_START + flash_addr);
    for (uint16_t i = 0; i < PROGMEM_PAGE_SIZE; i++) {
        dst[i] = page[i];
    }

    _PROTECTED_WRITE_SPM(NVMCTRL.CTRLA, NVMCTRL_CMD_PAGEERASEWRITE_gc);
    nvm_wait_flash();

    if ((NVMCTRL.STATUS & NVMCTRL_WRERROR_bm) != 0) {
        return false;
    }
    return true;
}

