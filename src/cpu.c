#include <emu6502/cpu.h>
#include <emu6502/utils.h>
#include <emu6502/memory.h>
#include <emu6502/decoding.h>
#include <emu6502/args.h>
#include <stdio.h>

#define NMI_VECTOR 0xfffa
#define RESET_VECTOR 0xfffc
#define BRK_VECTOR 0xfffe

#define CONDITIONAL_FLAG(cond, flag) do { \
            if(cond) reg.p |= (flag);     \
            else reg.p &= ~(flag);        \
        } while(0)
#define HAS_FLAG(flag) ((reg.p&(flag))==(flag))

typedef union mem_val {
    uint16_t w;
    uint8_t b;
} mem_val_t;

static void cpu_mode_get_addr(mem_val_t *, enum instr_address_mode);
static void cpu_mode_get_value(mem_val_t *, enum instr_address_mode);
static void cpu_mode_set_value(mem_val_t *, enum instr_address_mode, uint8_t);

static inline void set_reg(uint8_t *, int8_t);
static inline void compare(int8_t, int8_t);

static struct {
    uint8_t a, x, y;
    uint16_t pc;
    uint8_t s, p;
} reg = {0};

void cpu_init(void) {
    reg.s = 0xfd;
    reg.pc = memory_read_w(RESET_VECTOR);
    printf("Reset address: $%04x\n", reg.pc);
    reg.p |= FLAGS_UNUSED|FLAGS_BREAK|FLAGS_INTERRUPT|FLAGS_ZERO;
}

static void cpu_mode_get_addr(mem_val_t *v, enum instr_address_mode mode) {
    switch(mode) {
    default:
    case MODE_ACCUMULATOR:
    case MODE_IMPLIED:
        return;

    case MODE_IMMEDIATE:
        v->b = memory_read(reg.pc++);
        break;

    case MODE_ABSOLUTE:
        v->w = memory_read_w(reg.pc), reg.pc += 2;
        break;

    case MODE_ZERO_PAGE:
        v->w = memory_read(reg.pc++);
        break;

    case MODE_RELATIVE:
        /* relative PC on the next instruction */
        v->w = reg.pc+1 + (int8_t)memory_read(reg.pc), reg.pc++;
        break;

    case MODE_ABSOLUTE_INDIRECT:
        v->w = memory_read_w(memory_read_w(reg.pc)), reg.pc += 2;
        break;

    case MODE_ABSOLUTE_X:
        v->w = memory_read_w(reg.pc++) + reg.x;
        break;

    case MODE_ABSOLUTE_Y:
        v->w = memory_read_w(reg.pc++) + reg.y;
        break;

    case MODE_ZERO_PAGE_X:
        v->w = (uint8_t)(memory_read(reg.pc++) + reg.x);
        break;

    case MODE_ZERO_PAGE_Y:
        v->w = (uint8_t)(memory_read(reg.pc++) + reg.y);
        break;

    case MODE_ZERO_PAGE_INDIRECT_X:
        v->w = memory_read_w((uint8_t)(memory_read(reg.pc++) + reg.x));
        break;

    case MODE_ZERO_PAGE_INDIRECT_Y:
        v->w = (uint8_t)(memory_read_w(memory_read(reg.pc++)) + reg.y);
        break;
    }

    if(cmd_options.verbose >= 2) {
        if(mode == MODE_IMMEDIATE)
            printf("read value: $%02x\n", v->b);
        else
            printf("read address: $%04x\n", v->w);
    }
}

static void cpu_mode_get_value(mem_val_t *v, enum instr_address_mode mode) {
    switch(mode) {
    case MODE_ACCUMULATOR:
        v->b = reg.a;
        break;

    case MODE_IMPLIED:
        fprintf(stderr, "[Error] Trying to get the value of an implied"
                " argument\n");
        break;

    case MODE_IMMEDIATE:
        break;

    case MODE_ABSOLUTE_INDIRECT:
        fprintf(stderr, "[Error] Trying to get the value of an absolute"
                " indirect argument\n");
        break;

    default:
        v->b = memory_read(v->w);
        if(cmd_options.verbose >= 2)
            printf("read value: $%02x\n", v->b);
        break;
    }
}

static void cpu_mode_set_value(mem_val_t *v, enum instr_address_mode mode,
                               uint8_t val) {
    switch(mode) {
    case MODE_ACCUMULATOR:
        reg.a = val;
        break;

    case MODE_IMPLIED:
        fprintf(stderr, "[Error] Trying to write to an implied argument\n");
        break;

    case MODE_IMMEDIATE:
        fprintf(stderr, "[Error] Trying to set immediate value\n");
        break;

    case MODE_ABSOLUTE_INDIRECT:
        fprintf(stderr, "[Error] Trying to set absolute indirect value\n");
        break;

    default:
        memory_write(v->w, val);
        if(cmd_options.verbose >= 2)
            printf("wrote value to address: $%02x -> $%04x\n", val, v->w);
        CONDITIONAL_FLAG((int8_t)val < 0, FLAGS_NEGATIVE);
        CONDITIONAL_FLAG(val == 0, FLAGS_ZERO);
        break;
    }
}

static inline void set_reg(uint8_t *r, int8_t val) {
    *(int8_t *)r = val;
    CONDITIONAL_FLAG(val < 0, FLAGS_NEGATIVE);
    CONDITIONAL_FLAG(val == 0, FLAGS_ZERO);
}

static inline void compare(int8_t l, int8_t r) {
    if(l < r)
        reg.p |= FLAGS_NEGATIVE, reg.p &= ~(FLAGS_ZERO|FLAGS_CARRY);
    else if(l == r)
        reg.p |= FLAGS_ZERO|FLAGS_CARRY, reg.p &= ~FLAGS_NEGATIVE;
    else
        reg.p |= FLAGS_CARRY, reg.p &= ~(FLAGS_NEGATIVE|FLAGS_ZERO);
}

/* TODO: cycles? */
void cpu_step(void) {
    if(cmd_options.verbose >= 2) printf("-----\n");

    uint8_t opcode = memory_read(reg.pc++);
    const instr_t *instr = &instruction_table[opcode];
    mem_val_t v, tmp;
    cpu_mode_get_addr(&v, instr->mode);

    if(cmd_options.verbose >= 2)
        printf("running: %s %s\n",
               instr_type_str(instr->type), instr_mode_str(instr->mode));

#define GETVAL()    cpu_mode_get_value(&v, instr->mode)
#define GETTMPVAL() tmp.w = v.w, cpu_mode_get_value(&tmp, instr->mode)
#define SETVAL(val) cpu_mode_set_value(&v, instr->mode, val)
#define MODVAL(op)  tmp.w = v.w, cpu_mode_get_value(&v, instr->mode), \
        cpu_mode_set_value(&tmp, instr->mode, (op))
    switch(instr->type) {
    default:
        fprintf(stderr, "[Error] Illegal opcode $%02x\n", opcode);
        break;

    /* load and store */
    case OP_LDA: GETVAL(); set_reg(&reg.a, v.b); break;
    case OP_LDX: GETVAL(); set_reg(&reg.x, v.b); break;
    case OP_LDY: GETVAL(); set_reg(&reg.y, v.b); break;

    case OP_STA: SETVAL(reg.a); break;
    case OP_STX: SETVAL(reg.x); break;
    case OP_STY: SETVAL(reg.y); break;

    /* arithmetic */
    case OP_ADC:
        GETVAL();
        v.w = reg.a + v.b + HAS_FLAG(FLAGS_CARRY);
        CONDITIONAL_FLAG(v.w > 0xff, FLAGS_CARRY|FLAGS_OVERFLOW);
        set_reg(&reg.a, v.w&0xff);
        break;

    case OP_SBC:
        GETVAL();
        v.w = reg.a - v.b - !HAS_FLAG(FLAGS_CARRY);
        CONDITIONAL_FLAG(v.w > 0xff, FLAGS_CARRY|FLAGS_OVERFLOW);
        set_reg(&reg.a, v.w&0xff);
        break;

    /* increment and decrement */
    case OP_INC: MODVAL(v.b+1); break;
    case OP_INX: GETVAL(); set_reg(&reg.x, reg.x+1); break;
    case OP_INY: GETVAL(); set_reg(&reg.y, reg.y+1); break;

    case OP_DEC: MODVAL(v.b-1); break;
    case OP_DEX: GETVAL(); set_reg(&reg.x, reg.x-1); break;
    case OP_DEY: GETVAL(); set_reg(&reg.y, reg.y-1); break;

    /* shift and rotate */
    /* TODO: I have literally no idea how the flags for these instructions
     * should work */
    case OP_ASL:
        GETTMPVAL();
        CONDITIONAL_FLAG(tmp.b&0x80, FLAGS_CARRY);
        SETVAL(tmp.b<<1);
        break;

    case OP_LSR:
        GETTMPVAL();
        CONDITIONAL_FLAG(tmp.b&0x80, FLAGS_CARRY);
        SETVAL(tmp.b>>1);
        break;

    case OP_ROL:
        GETTMPVAL();
        SETVAL(tmp.b<<1 | HAS_FLAG(FLAGS_CARRY));
        CONDITIONAL_FLAG(tmp.b&0x80, FLAGS_CARRY);
        break;

    case OP_ROR:
        GETTMPVAL();
        SETVAL(tmp.b>>1 | HAS_FLAG(FLAGS_CARRY)<<7);
        CONDITIONAL_FLAG(tmp.b&0x01, FLAGS_CARRY);
        break;

    /* logic */
    case OP_AND: GETVAL(); set_reg(&reg.a, reg.a&v.b); break;
    case OP_ORA: GETVAL(); set_reg(&reg.a, reg.a|v.b); break;
    case OP_EOR: GETVAL(); set_reg(&reg.a, reg.a^v.b); break;

    /* compare and test bit */
    case OP_CMP: GETVAL(); compare(reg.a, v.b); break;
    case OP_CPX: GETVAL(); compare(reg.x, v.b); break;
    case OP_CPY: GETVAL(); compare(reg.y, v.b); break;
    case OP_BIT:
        GETVAL();
        CONDITIONAL_FLAG(v.b&0x80, FLAGS_NEGATIVE);
        CONDITIONAL_FLAG(v.b&0x40, FLAGS_OVERFLOW);
        CONDITIONAL_FLAG(!(v.b&reg.a), FLAGS_ZERO);
        break;

    /* branch */
    case OP_BCC: GETVAL(); if(!HAS_FLAG(FLAGS_CARRY))    reg.pc = v.w; break;
    case OP_BCS: GETVAL(); if(HAS_FLAG(FLAGS_CARRY))     reg.pc = v.w; break;
    case OP_BNE: GETVAL(); if(!HAS_FLAG(FLAGS_ZERO))     reg.pc = v.w; break;
    case OP_BEQ: GETVAL(); if(HAS_FLAG(FLAGS_ZERO))      reg.pc = v.w; break;
    case OP_BPL: GETVAL(); if(!HAS_FLAG(FLAGS_NEGATIVE)) reg.pc = v.w; break;
    case OP_BMI: GETVAL(); if(HAS_FLAG(FLAGS_NEGATIVE))  reg.pc = v.w; break;
    case OP_BVC: GETVAL(); if(!HAS_FLAG(FLAGS_OVERFLOW)) reg.pc = v.w; break;
    case OP_BVS: GETVAL(); if(HAS_FLAG(FLAGS_OVERFLOW))  reg.pc = v.w; break;

    /* transfer */
    case OP_TAX: set_reg(&reg.x, reg.a); break;
    case OP_TXA: set_reg(&reg.a, reg.x); break;
    case OP_TAY: set_reg(&reg.y, reg.a); break;
    case OP_TYA: set_reg(&reg.a, reg.y); break;
    case OP_TSX: set_reg(&reg.x, reg.s); break;
    /* NOTE: TXS does not set any flags */
    case OP_TXS: reg.s = reg.x; break;

    /* stack */
    case OP_PHA: memory_write(0x100 + reg.s--, reg.a); break;
    case OP_PLA: set_reg(&reg.a, memory_read(0x100 + ++reg.s)); break;
    case OP_PHP: memory_write(0x100 + reg.s--, reg.p); break;
    case OP_PLP: reg.p = memory_read(0x100 + ++reg.s); break;

    /* subroutines and jump */
    case OP_JMP: reg.pc = v.w; break;
    case OP_JSR:
        memory_write_w(reg.s-1, reg.pc-1), reg.s -= 2, reg.pc = v.w;
        break;
    case OP_RTS: reg.pc = memory_read_w(reg.s+1) + 1, reg.s += 2; break;
    case OP_RTI:
        reg.p = memory_read(reg.s+1);
        reg.pc = memory_read_w(reg.s+2);
        reg.s += 3;
        break;

    /* set and clear */
    case OP_CLC: reg.p &= ~FLAGS_CARRY; break;
    case OP_SEC: reg.p |=  FLAGS_CARRY; break;
    case OP_CLD: reg.p &= ~FLAGS_DECIMAL; break;
    case OP_SED: reg.p |=  FLAGS_DECIMAL; break;
    case OP_CLI: reg.p &= ~FLAGS_INTERRUPT; break;
    case OP_SEI: reg.p |=  FLAGS_INTERRUPT; break;
    case OP_CLV: reg.p &= ~FLAGS_OVERFLOW; break;

    /* miscellaneous */
    case OP_BRK:
        memory_write_w(reg.s-1, reg.pc);
        memory_write(reg.s-2, reg.p);
        reg.s -= 3;
        reg.p |= FLAGS_BREAK|FLAGS_INTERRUPT;
        reg.pc = memory_read_w(BRK_VECTOR);
        break;

    case OP_NOP: break;
    }
#undef GETVAL
#undef GETTMPVAL
#undef SETVAL
#undef MODVAL
}
