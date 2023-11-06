#include <emu6502/memory.h>
#include <string.h>

#define MIN(a, b) ((a)<(b)?(a):(b))

static uint8_t emu_ram[0x800] = {0};
static uint8_t emu_prg_rom[0xbfe0] = {0};

static uint8_t data_bus = 0;

static inline void memory_read_ram(uint16_t);
static inline void memory_read_prg_rom(uint16_t);

static inline void memory_write_ram(uint16_t);
static inline void memory_write_prg_rom(uint16_t);

static inline void memory_read_ram(uint16_t addr) {
    data_bus = emu_ram[addr&0x7ff];
}

static inline void memory_read_prg_rom(uint16_t addr) {
    data_bus = emu_prg_rom[(addr-0x4020)];
}

static inline void memory_write_ram(uint16_t addr) {
    emu_ram[addr&0x7ff] = data_bus;
}

static inline void memory_write_prg_rom(uint16_t addr) {
    emu_prg_rom[(addr-0x4020)] = data_bus;
}

uint8_t memory_read(uint16_t addr) {
    if(addr < 0x2000)
        memory_read_ram(addr);
    else if(addr >= 0x4020)
        memory_read_prg_rom(addr);
    /* else open bus */
    return data_bus;
}

uint16_t memory_read_w(uint16_t addr) {
    union {
        uint16_t w;
        struct {
            uint8_t l;
            uint8_t h;
        };
    } out;
    out.l = memory_read(addr);
    out.h = memory_read(addr+1);
    return out.w;
}

void memory_write(uint16_t addr, uint8_t val) {
    data_bus = val;
    if(addr < 0x2000)
        memory_write_ram(addr);
    else if(addr >= 0x4020)
        memory_write_prg_rom(addr);
}

void memory_load_rom(uint8_t *data, size_t sz) {
    (void)memcpy(&emu_prg_rom, data, MIN(sz, sizeof emu_prg_rom));
}
