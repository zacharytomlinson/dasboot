#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint8_t manufacturer_id;
    uint8_t memory_type;
    uint8_t capacity;
} w25q_jedec_id_t;

void w25q_pins_init(void);
w25q_jedec_id_t w25q_read_jedec_id(void);
bool w25q_probe(void);

bool w25q_read(uint32_t addr, uint8_t *buf, uint16_t len);
