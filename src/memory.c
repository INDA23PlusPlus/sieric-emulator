#include <emu6502/memory.h>
#include <string.h>
#include <endianness.h>

#define MIN(a, b) ((a)<(b)?(a):(b))

static uint8_t emu_ram[0x800] = {0};
static uint8_t emu_prg_rom[0xbfe0] = {0};

static const memory_map_entry_t *memory_map[0x1000] = {0};

static uint8_t data_bus = 0;

static void memory_ram_read(uint8_t *, uint16_t);
static void memory_ram_write(uint8_t *, uint16_t);
static const memory_map_entry_t memory_ram_entry;

static void memory_prg_rom_read(uint8_t *, uint16_t);
static void memory_prg_rom_write(uint8_t *, uint16_t);
static const memory_map_entry_t memory_prg_rom_entry;

static void memory_ram_read(uint8_t *bus, uint16_t addr) {
    *bus = emu_ram[addr&0x7ff];
}

static void memory_ram_write(uint8_t *bus, uint16_t addr) {
    emu_ram[addr&0x7ff] = *bus;
}

static const memory_map_entry_t memory_ram_entry = {
    .read = memory_ram_read,
    .write = memory_ram_write,
};

static void memory_prg_rom_read(uint8_t *bus, uint16_t addr) {
    *bus = emu_prg_rom[(addr-0x4020)];
}

static void memory_prg_rom_write(uint8_t *bus, uint16_t addr) {
    emu_prg_rom[(addr-0x4020)] = *bus;
}

static const memory_map_entry_t memory_prg_rom_entry = {
    .read = memory_prg_rom_read,
    .write = memory_prg_rom_write,
};

inline void memory_map_page(const memory_map_entry_t *const entry, uint16_t page) {
    memory_map[page>>4] = entry;
}

void memory_init(void) {
    memset(memory_map, 0, sizeof memory_map);
    uint32_t page;
    for(page = 0x0; page < 0x2000; page += 0x10)
        memory_map_page(&memory_ram_entry, page);
    for(page = 0x4020; page < 0x10000; page += 0x10)
        memory_map_page(&memory_prg_rom_entry, page);
}

uint8_t memory_read(uint16_t addr) {
    const memory_map_entry_t *entry = memory_map[addr>>4];
    if(entry && entry->read) entry->read(&data_bus, addr);
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
    return end_le16toh(out.w);
}

void memory_write(uint16_t addr, uint8_t val) {
    const memory_map_entry_t *entry = memory_map[addr>>4];
    data_bus = val;
    if(entry && entry->write) entry->write(&data_bus, addr);
}

void memory_write_w(uint16_t addr, uint16_t val) {
    union {
        uint16_t w;
        struct {
            uint8_t l;
            uint8_t h;
        };
    } v;
    v.w = end_htole16(val);
    memory_write(addr, v.l);
    memory_write(addr+1, v.h);
}

void memory_load_rom(uint8_t *data, size_t sz) {
    (void)memcpy(emu_prg_rom, data, MIN(sz, sizeof emu_prg_rom));
}

void memory_load_rom_addr(uint8_t *data, size_t sz, uint16_t addr) {
    if(addr < 0x4020) return;
    addr -= 0x4020;
    (void)memcpy(emu_prg_rom + addr, data,
                 MIN(sz, sizeof emu_prg_rom - addr));
}
