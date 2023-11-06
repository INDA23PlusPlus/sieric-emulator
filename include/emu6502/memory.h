#ifndef EMU6502_MEMORY_H_
#define EMU6502_MEMORY_H_

#include <stdlib.h>
#include <stdint.h>

typedef struct memory_map_entry {
    void (*read)(uint8_t *, uint16_t);
    void (*write)(uint8_t *, uint16_t);
} memory_map_entry_t;

void memory_map_page(const memory_map_entry_t *const, uint16_t);
void memory_init(void);
uint8_t memory_read(uint16_t);
uint16_t memory_read_w(uint16_t);
void memory_write(uint16_t, uint8_t);
void memory_write_w(uint16_t, uint16_t);
void memory_load_rom(uint8_t *, size_t);
void memory_load_rom_addr(uint8_t *, size_t, uint16_t);

#endif /* EMU6502_MEMORY_H_ */
