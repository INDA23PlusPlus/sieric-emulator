#include <emu6502/utils.h>
#include <emu6502/args.h>
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

    printf("Verbosity: %u\n", cmd_options.verbose);
    for(int i = 0; i < argc; ++i)
        printf("%d: %s\n", i, argv[i]);

    exit(EXIT_SUCCESS);
}
