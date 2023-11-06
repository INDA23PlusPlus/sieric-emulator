#ifndef EMU6502_DECODING_H_
#define EMU6502_DECODING_H_

#include <stdint.h>

enum instr_type {
#define T(t, v) OP_##t = v,
#include <emu6502/__instr.h>
#undef T
};

enum instr_address_mode {
MODE_ACCUMULATOR = 0,
MODE_IMPLIED,
MODE_IMMEDIATE,
MODE_ABSOLUTE,
MODE_ZERO_PAGE,
MODE_RELATIVE,
MODE_ABSOLUTE_INDIRECT,
MODE_ABSOLUTE_X,
MODE_ABSOLUTE_Y,
MODE_ZERO_PAGE_X,
MODE_ZERO_PAGE_Y,
MODE_ZERO_PAGE_INDIRECT_X,
MODE_ZERO_PAGE_INDIRECT_Y,

/* shorthand because lazy */
MODE_A = 0,
MODE_i,
MODE_IMM,
MODE_a,
MODE_zp,
MODE_r,
MODE_a_IN,
MODE_a_x,
MODE_a_y,
MODE_zp_x,
MODE_zp_y,
MODE_zp_x_IN,
MODE_zp_y_IN,
};

typedef struct instr {
    enum instr_type type;
    enum instr_address_mode mode;
} instr_t;

extern const instr_t instruction_table[0x100];

const char *instr_type_str(enum instr_type);
const char *instr_mode_str(enum instr_address_mode);

#endif /* EMU6502_DECODING_H_ */
