#include <emu6502/decoding.h>
#include <stdlib.h>
#include <stddef.h>

static const struct instrtypestr {
#define T(t, v) char str##t[sizeof(#t)];
#include <emu6502/__instr.h>
#undef T
} instrtypestr = {
#define T(t, v) #t,
#include <emu6502/__instr.h>
#undef T
};

static const uint8_t instrtypeidx[] = {
#define T(t, v) [(v)] = offsetof(struct instrtypestr, str##t),
#include <emu6502/__instr.h>
#undef T
};

const instr_t instruction_table[0x100] = {
/* 0x00 */
[0x00] = {OP_BRK, MODE_i},
[0x01] = {OP_ORA, MODE_zp_x_IN},
[0x05] = {OP_ORA, MODE_zp},
[0x06] = {OP_ASL, MODE_zp},
[0x08] = {OP_PHP, MODE_i},
[0x09] = {OP_ORA, MODE_IMM},
[0x0a] = {OP_ASL, MODE_A},
[0x0d] = {OP_ORA, MODE_a},
[0x0e] = {OP_ASL, MODE_a},

/* 0x10 */
[0x10] = {OP_BPL, MODE_r},
[0x11] = {OP_ORA, MODE_zp_y_IN},
[0x15] = {OP_ORA, MODE_zp_x},
[0x16] = {OP_ASL, MODE_zp_x},
[0x18] = {OP_CLC, MODE_i},
[0x19] = {OP_ORA, MODE_a_y},
[0x1d] = {OP_ORA, MODE_a_x},
[0x1e] = {OP_ASL, MODE_a_x},

/* 0x20 */
[0x20] = {OP_JSR, MODE_a},
[0x21] = {OP_AND, MODE_zp_x_IN},
[0x24] = {OP_BIT, MODE_zp},
[0x25] = {OP_AND, MODE_zp},
[0x26] = {OP_ROL, MODE_zp},
[0x28] = {OP_PLP, MODE_i},
[0x29] = {OP_AND, MODE_IMM},
[0x2a] = {OP_ROL, MODE_A},
[0x2c] = {OP_BIT, MODE_a},
[0x2d] = {OP_AND, MODE_a},
[0x2e] = {OP_ROL, MODE_a},

/* 0x30 */
[0x30] = {OP_BMI, MODE_r},
[0x31] = {OP_AND, MODE_zp_y_IN},
[0x35] = {OP_AND, MODE_zp_x},
[0x36] = {OP_ROL, MODE_zp_x},
[0x38] = {OP_SEC, MODE_i},
[0x39] = {OP_AND, MODE_a_y},
[0x3d] = {OP_AND, MODE_a_x},
[0x3e] = {OP_ROL, MODE_a_x},

/* 0x40 */
[0x40] = {OP_RTI, MODE_i},
[0x41] = {OP_EOR, MODE_zp_x_IN},
[0x45] = {OP_EOR, MODE_zp},
[0x46] = {OP_LSR, MODE_zp},
[0x48] = {OP_PHA, MODE_i},
[0x49] = {OP_EOR, MODE_IMM},
[0x4a] = {OP_LSR, MODE_A},
[0x4c] = {OP_JMP, MODE_a},
[0x4d] = {OP_EOR, MODE_a},
[0x4e] = {OP_LSR, MODE_a},

/* 0x50 */
[0x50] = {OP_BVC, MODE_r},
[0x51] = {OP_EOR, MODE_zp_y_IN},
[0x55] = {OP_EOR, MODE_zp_x},
[0x56] = {OP_LSR, MODE_zp_x},
[0x58] = {OP_CLI, MODE_i},
[0x59] = {OP_EOR, MODE_a_y},
[0x5d] = {OP_EOR, MODE_a_x},
[0x5e] = {OP_LSR, MODE_a_x},

/* 0x60 */
[0x60] = {OP_RTS, MODE_i},
[0x61] = {OP_ADC, MODE_zp_x_IN},
[0x65] = {OP_ADC, MODE_zp},
[0x66] = {OP_ROR, MODE_zp_x},
[0x68] = {OP_PLA, MODE_i},
[0x69] = {OP_ADC, MODE_IMM},
[0x6a] = {OP_ROR, MODE_A},
[0x6c] = {OP_JMP, MODE_a_IN},
[0x6d] = {OP_ADC, MODE_a},
[0x6e] = {OP_ROR, MODE_a},

/* 0x70 */
[0x70] = {OP_BVS, MODE_r},
[0x71] = {OP_ADC, MODE_zp_y_IN},
[0x75] = {OP_ADC, MODE_zp_x},
[0x76] = {OP_ROR, MODE_zp_x},
[0x78] = {OP_SEI, MODE_i},
[0x79] = {OP_ADC, MODE_a_y},
[0x7d] = {OP_ADC, MODE_a_x},
[0x7e] = {OP_ROR, MODE_a_x},

/* 0x80 */
[0x81] = {OP_STA, MODE_zp_x_IN},
[0x84] = {OP_STY, MODE_zp},
[0x85] = {OP_STA, MODE_zp},
[0x86] = {OP_STX, MODE_zp},
[0x88] = {OP_DEY, MODE_i},
[0x89] = {OP_BIT, MODE_IMM},
[0x8a] = {OP_TXA, MODE_i},
[0x8c] = {OP_STY, MODE_a},
[0x8d] = {OP_STA, MODE_a},
[0x8e] = {OP_STX, MODE_a},

/* 0x90 */
[0x90] = {OP_BCC, MODE_r},
[0x91] = {OP_STA, MODE_zp_y_IN},
[0x94] = {OP_STY, MODE_zp_x},
[0x95] = {OP_STA, MODE_zp_x},
[0x96] = {OP_STX, MODE_zp_y},
[0x98] = {OP_TYA, MODE_i},
[0x99] = {OP_STA, MODE_a_y},
[0x9a] = {OP_TXS, MODE_i},
[0x9d] = {OP_STA, MODE_a_x},

/* 0xa0 */
[0xa0] = {OP_LDY, MODE_IMM},
[0xa1] = {OP_LDA, MODE_zp_x_IN},
[0xa2] = {OP_LDX, MODE_IMM},
[0xa4] = {OP_LDY, MODE_zp},
[0xa5] = {OP_LDA, MODE_zp},
[0xa6] = {OP_LDX, MODE_zp},
[0xa8] = {OP_TAY, MODE_i},
[0xa9] = {OP_LDA, MODE_IMM},
[0xaa] = {OP_TAX, MODE_i},
[0xac] = {OP_LDY, MODE_a},
[0xad] = {OP_LDA, MODE_a},
[0xae] = {OP_LDX, MODE_a},

/* 0xb0 */
[0xb0] = {OP_BCS, MODE_r},
[0xb1] = {OP_LDA, MODE_zp_y_IN},
[0xb4] = {OP_LDY, MODE_zp_x},
[0xb5] = {OP_LDA, MODE_zp_x},
[0xb6] = {OP_LDX, MODE_zp_x},
[0xb8] = {OP_CLV, MODE_i},
[0xb9] = {OP_LDA, MODE_a_y},
[0xba] = {OP_TSX, MODE_i},
[0xbc] = {OP_LDY, MODE_a_x},
[0xbd] = {OP_LDA, MODE_a_x},
[0xbe] = {OP_LDX, MODE_a_y},

/* 0xc0 */
[0xc0] = {OP_CPY, MODE_IMM},
[0xc1] = {OP_CMP, MODE_zp_x_IN},
[0xc4] = {OP_CPY, MODE_zp},
[0xc5] = {OP_CMP, MODE_zp},
[0xc6] = {OP_DEC, MODE_zp},
[0xc8] = {OP_INY, MODE_i},
[0xc9] = {OP_CMP, MODE_IMM},
[0xca] = {OP_DEX, MODE_i},
[0xcc] = {OP_CPY, MODE_a},
[0xcd] = {OP_CMP, MODE_a},
[0xce] = {OP_DEC, MODE_a},

/* 0xd0 */
[0xd0] = {OP_BNE, MODE_r},
[0xd1] = {OP_CMP, MODE_zp_y_IN},
[0xd5] = {OP_CMP, MODE_zp_x},
[0xd6] = {OP_DEC, MODE_zp_x},
[0xd8] = {OP_CLD, MODE_i},
[0xd9] = {OP_CMP, MODE_a_y},
[0xdd] = {OP_CMP, MODE_a_x},
[0xde] = {OP_DEC, MODE_a_x},

/* 0xe0 */
[0xe0] = {OP_CPX, MODE_IMM},
[0xe1] = {OP_SBC, MODE_zp_x_IN},
[0xe4] = {OP_CPX, MODE_zp},
[0xe5] = {OP_SBC, MODE_zp},
[0xe6] = {OP_INC, MODE_zp},
[0xe8] = {OP_INX, MODE_i},
[0xe9] = {OP_SBC, MODE_IMM},
[0xea] = {OP_NOP, MODE_i},
[0xec] = {OP_CPX, MODE_a},
[0xed] = {OP_SBC, MODE_a},
[0xee] = {OP_INC, MODE_a},

/* 0xf0 */
[0xf0] = {OP_BEQ, MODE_r},
[0xf1] = {OP_SBC, MODE_zp_y_IN},
[0xf5] = {OP_SBC, MODE_zp_x},
[0xf6] = {OP_INC, MODE_zp_x},
[0xf8] = {OP_SED, MODE_i},
[0xf9] = {OP_SBC, MODE_a_y},
[0xfd] = {OP_SBC, MODE_a_x},
[0xfe] = {OP_INC, MODE_a_x},
};

const char *instr_type_str(enum instr_type type) {
    if((size_t)type >= sizeof instrtypeidx / sizeof *instrtypeidx) type = 0;
    return (char *)&instrtypestr + instrtypeidx[type];
}

const char *instr_mode_str(enum instr_address_mode mode) {
    static const char *names[] = {
    [MODE_A] = "A",
    [MODE_i] = "i",
    [MODE_IMM] = "#",
    [MODE_a] = "a",
    [MODE_zp] = "zp",
    [MODE_r] = "r",
    [MODE_a_IN] = "(a)",
    [MODE_a_x] = "a,x",
    [MODE_a_y] = "a,y",
    [MODE_zp_x] = "zp,x",
    [MODE_zp_y] = "zp,y",
    [MODE_zp_x_IN] = "(zp,x)",
    [MODE_zp_y_IN] = "(zp),y",
    };
    if(mode >= sizeof names / sizeof *names) return "UNKNOWN";
    else return names[mode];
}
