#include <emu6502/utils.h>
#include <emu6502/args.h>
#include <emu6502/cpu.h>
#include <emu6502/memory.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#define PROGRAM_NAME "emu6502"

struct cmd_options cmd_options = {
    .verbose = 0,
};

const char *help_str = ""
"Usage: " PROGRAM_NAME " [option]... rom\n"
"\n"
"Options:\n"
"  -v, --verbose              increment the verbosity level\n"
"  -h, --help                 print this help message\n"
;

int main(int argc, char *argv[]) {
    int c;

    for(;;) {
        int longind;

        static struct option long_opts[] = {
        {"verbose", no_argument, NULL, 'v'},
        {"help", no_argument, NULL, 'h'},
        {0, 0, 0, 0},
        };

        if((c = getopt_long(argc, argv, "vh", long_opts, &longind)) == -1)
           break;

        switch(c) {
        case 'v':
            cmd_options.verbose++;
            break;

        case 'h':
            die(help_str);

        case '?':
            break;

        default:
            printf("?? getopt returned character code 0%o ??\n", c);
            break;
        }
    }

    argv += optind;
    if((argc -= optind) < 1) die(help_str);

    {
        uint8_t test_rom[] = "\x69";
        memory_load_rom(test_rom, sizeof test_rom - 1);
        memory_write(0xfffc, 0x20);
        memory_write(0xfffd, 0x40);
    }
    cpu_init();
    cpu_step();

    exit(EXIT_SUCCESS);
}
