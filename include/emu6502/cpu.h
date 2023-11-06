#ifndef EMU6502_CPU_H_
#define EMU6502_CPU_H_

#include <stdint.h>

#define FLAGS_CARRY      (1<<0)
#define FLAGS_ZERO       (1<<1)
#define FLAGS_INTERRUPT  (1<<2)
#define FLAGS_DECIMAL    (1<<3)
#define FLAGS_BREAK      (1<<4)
#define FLAGS_UNUSED     (1<<5)
#define FLAGS_OVERFLOW   (1<<6)
#define FLAGS_NEGATIVE   (1<<7)

extern int cpu_halt;

void cpu_init(void);
void cpu_step(void);

#endif /* EMU6502_CPU_H_ */
