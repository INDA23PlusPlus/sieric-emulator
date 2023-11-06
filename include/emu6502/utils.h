#ifndef EMU6502_UTILS_H_
#define EMU6502_UTILS_H_

#include <features.h>
#include <sys/cdefs.h>

#ifdef __GNUC__
#define __unused __attribute__((unused))
#else
#define __unused
#endif

#ifdef __GNUC__
#define __fallthrough __attribute__((fallthrough))
#else
#define __fallthrough
#endif

_Noreturn void die(const char *);

#endif /* EMU6502_UTILS_H_ */
