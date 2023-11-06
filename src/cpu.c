#include <emu6502/cpu.h>
#include <emu6502/utils.h>
#include <emu6502/memory.h>
#include <emu6502/decoding.h>
#include <stdio.h>

#define NMI_VECTOR 0xfffa
#define RESET_VECTOR 0xfffc
#define BRK_VECTOR 0xfffe

static struct {
    uint8_t a, x, y;
    uint16_t pc;
    uint8_t s, p;
} reg = {0};

void cpu_init(void) {
    reg.s = 0xfd;
    reg.pc = memory_read_w(RESET_VECTOR);
    printf("Reset address: $%04x\n", reg.pc);
    reg.p |= 1<<5;
}

/* TODO: cycles? */
void cpu_step(void) {
    uint8_t opcode = memory_read(reg.pc++);
    const instr_t *instr = &instruction_table[opcode];
    union {
        uint16_t w;
        uint8_t b;
    } v;

    switch(instr->type) {
    default:
        fprintf(stderr, "Illegal opcode $%02x\n", opcode);
        __fallthrough;
    case OP_BRK:
        reg.pc = memory_read(BRK_VECTOR);
        break;
    }
}
