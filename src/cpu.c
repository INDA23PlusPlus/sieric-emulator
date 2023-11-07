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

static void memory_io_read(uint8_t *, uint16_t);
static void memory_io_write(uint8_t *, uint16_t);
static const memory_map_entry_t memory_io_entry;

static struct {
    uint8_t a, x, y;
    uint16_t pc;
    uint8_t s, p;
} reg = {0};

int cpu_halt = 0;

static void memory_io_read(uint8_t *bus, uint16_t addr) {
    switch(addr) {
    case 0x3ff0:
        *bus = (uint8_t)getchar();
        break;
    }
}

static void memory_io_write(uint8_t *bus, uint16_t addr) {
    switch(addr) {
    case 0x3ff0:
        putchar(*bus);
        break;

    case 0x3fff:
        if(*bus == 0)
            cpu_init();
        else if(*bus == 1)
            cpu_halt = 1;
        break;
    }
}

static const memory_map_entry_t memory_io_entry = {
    .read = memory_io_read,
    .write = memory_io_write,
};

void cpu_init(void) {
    memory_init();
    memory_map_page(&memory_io_entry, 0x3ff0);
    reg.s = 0xff;
    reg.pc = memory_read_w(RESET_VECTOR);
    if(cmd_options.verbose >= 1)
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
        v->w = memory_read_w(reg.pc) + reg.x, reg.pc += 2;
        break;

    case MODE_ABSOLUTE_Y:
        v->w = memory_read_w(reg.pc) + reg.y, reg.pc += 2;
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
    uint8_t opcode = memory_read(reg.pc++);
    const instr_t *instr = &instruction_table[opcode];
    mem_val_t v, tmp;

    if(cmd_options.verbose >= 2)
        printf("-----\n$%04x: %s %s\n", reg.pc-1,
               instr_type_str(instr->type), instr_mode_str(instr->mode));

    cpu_mode_get_addr(&v, instr->mode);

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
    case OP_INX: set_reg(&reg.x, reg.x+1); break;
    case OP_INY: set_reg(&reg.y, reg.y+1); break;

    case OP_DEC: MODVAL(v.b-1); break;
    case OP_DEX: set_reg(&reg.x, reg.x-1); break;
    case OP_DEY: set_reg(&reg.y, reg.y-1); break;

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
    case OP_BCC: if(!HAS_FLAG(FLAGS_CARRY))    reg.pc = v.w; break;
    case OP_BCS: if(HAS_FLAG(FLAGS_CARRY))     reg.pc = v.w; break;
    case OP_BNE: if(!HAS_FLAG(FLAGS_ZERO))     reg.pc = v.w; break;
    case OP_BEQ: if(HAS_FLAG(FLAGS_ZERO))      reg.pc = v.w; break;
    case OP_BPL: if(!HAS_FLAG(FLAGS_NEGATIVE)) reg.pc = v.w; break;
    case OP_BMI: if(HAS_FLAG(FLAGS_NEGATIVE))  reg.pc = v.w; break;
    case OP_BVC: if(!HAS_FLAG(FLAGS_OVERFLOW)) reg.pc = v.w; break;
    case OP_BVS: if(HAS_FLAG(FLAGS_OVERFLOW))  reg.pc = v.w; break;

    /* transfer */
    case OP_TAX: set_reg(&reg.x, reg.a); break;
    case OP_TXA: set_reg(&reg.a, reg.x); break;
    case OP_TAY: set_reg(&reg.y, reg.a); break;
    case OP_TYA: set_reg(&reg.a, reg.y); break;
    case OP_TSX: set_reg(&reg.x, reg.s); break;
    /* NOTE: TXS does not set any flags */
    case OP_TXS: reg.s = reg.x; break;

    /* stack */
    case OP_PHA: memory_write(0x100 + (uint8_t)(reg.s--), reg.a); break;
    case OP_PLA: set_reg(&reg.a, memory_read(0x100 + (uint8_t)(++reg.s)));
        break;
    case OP_PHP: memory_write(0x100 + (uint8_t)(reg.s--), reg.p); break;
    case OP_PLP: reg.p = memory_read(0x100 + (uint8_t)(++reg.s)); break;

    /* subroutines and jump */
    case OP_JMP: reg.pc = v.w; break;
    case OP_JSR:
        memory_write_w(0x100 + (uint8_t)(reg.s-1), reg.pc-1);
        reg.s -= 2, reg.pc = v.w;
        break;
    case OP_RTS:
        reg.pc = memory_read_w(0x100 + (uint8_t)(reg.s+1)) + 1;
        reg.s += 2;
        break;
    case OP_RTI:
        reg.p = memory_read(0x100 + (uint8_t)(reg.s+1));
        reg.pc = memory_read_w(0x100 + (uint8_t)(reg.s+2));
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

void cpu_dump(void) {
    char buf[8];
    for(uint8_t i = 0; i < 8; ++i)
        buf[7-i] = reg.p&(1<<i) ? '1' : '0';
    printf("-----\n"
           "PC: $%04x\n"
           "A: $%02x\n"
           "X: $%02x\n"
           "Y: $%02x\n"
           "S: $%02x\n"
           "    NV-BDIZC\n"
           "P: %%%8s\n", reg.pc, reg.a, reg.x, reg.y, reg.s, buf);
}
