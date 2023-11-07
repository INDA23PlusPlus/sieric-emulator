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
    .step = 0,
};

const char *help_str = ""
"Usage: " PROGRAM_NAME " [option]... rom\n"
"\n"
"Options:\n"
"  -v, --verbose              increment the verbosity level\n"
"  -h, --help                 print this help message\n"
"  -d, --debug                start in debugging mode\n"
;

int main(int argc, char *argv[]) {
    int ret = EXIT_SUCCESS;

    for(;;) {
        int longind, c;

        static struct option long_opts[] = {
        {"verbose", no_argument, NULL, 'v'},
        {"help", no_argument, NULL, 'h'},
        {"debug", no_argument, NULL, 'd'},
        {0, 0, 0, 0},
        };

        if((c = getopt_long(argc, argv, "vhd", long_opts, &longind)) == -1)
           break;

        switch(c) {
        case 'v':
            cmd_options.verbose++;
            break;

        case 'd':
            cmd_options.step = 1;
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

    FILE *f;
    if(!(f = fopen(argv[0], "rb"))) {
        perror("fopen");
        ret = EXIT_FAILURE;
        goto ret;
    }

    fseek(f, 0, SEEK_END);
    size_t rom_sz = ftell(f);
    rewind(f);

    {
        uint8_t rom[rom_sz];
        if(fread(rom, 1, rom_sz, f) != rom_sz) {
            fclose(f);
            perror("fread");
            ret = EXIT_FAILURE;
            goto ret;
        }
        fclose(f);
        /* TODO: load ROM from FILE * directly */
        memory_load_rom_addr(rom, rom_sz, 0x8000);
    }
    cpu_init();

    if(cmd_options.step)
        do {
            cpu_step();
            cpu_dump();
        } while(!cpu_halt && fgetc(stdin) != 'q');
    else
        while(!cpu_halt) cpu_step();

ret:
    exit(ret);
}
