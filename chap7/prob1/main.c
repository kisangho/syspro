#define _XOPEN_SOURCE 700
#include "student.h"
#include "../record_io.h"

int main(int argc, char *argv[])
{
    int writable;
    char *record_arguments[3];

    if (argc != 3 || (strcmp(argv[1], "-r") != 0 &&
                      strcmp(argv[1], "-w") != 0)) {
        fprintf(stderr, "Usage: %s -r|-w test.txt\n", argv[0]);
        return EXIT_FAILURE;
    }
    writable = strcmp(argv[1], "-w") == 0;
    record_arguments[0] = argv[0];
    record_arguments[1] = argv[2];
    record_arguments[2] = NULL;
    return run_records(2, record_arguments, writable, 0);
}
