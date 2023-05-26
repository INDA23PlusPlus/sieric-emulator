#include <emu6502/utils.h>
#include <stdio.h>
#include <stdlib.h>

_Noreturn void die(const char *str) {
    printf("%s", str);
    exit(EXIT_FAILURE);
}
