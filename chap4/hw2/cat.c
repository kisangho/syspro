#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int print_file(FILE *stream, int numbered, unsigned long *line,
                      int *line_start)
{
    int character;

    while ((character = fgetc(stream)) != EOF) {
        if (numbered && *line_start && printf("%6lu\t", (*line)++) < 0)
            return -1;
        if (fputc(character, stdout) == EOF)
            return -1;
        *line_start = character == '\n';
    }
    return ferror(stream) ? -1 : 0;
}

int main(int argc, char *argv[])
{
    int numbered = 0;
    int first_file = 1;
    int status = EXIT_SUCCESS;
    int line_start = 1;
    unsigned long line = 1;

    while (first_file < argc && argv[first_file][0] == '-') {
        if (strcmp(argv[first_file], "--") == 0) {
            first_file++;
            break;
        }
        if (strcmp(argv[first_file], "-") == 0)
            break;
        if (strcmp(argv[first_file], "-n") != 0) {
            fprintf(stderr, "Usage: %s [-n] [--] [file ...]\n", argv[0]);
            return EXIT_FAILURE;
        }
        numbered = 1;
        first_file++;
    }
    if (first_file == argc) {
        if (print_file(stdin, numbered, &line, &line_start) == -1) {
            perror("stdin/stdout");
            status = EXIT_FAILURE;
        }
    }
    for (int index = first_file; index < argc; index++) {
        FILE *stream = strcmp(argv[index], "-") == 0
                       ? stdin : fopen(argv[index], "r");
        if (stream == NULL) {
            perror(argv[index]);
            status = EXIT_FAILURE;
            continue;
        }
        if (print_file(stream, numbered, &line, &line_start) == -1) {
            perror(argv[index]);
            status = EXIT_FAILURE;
        }
        if (stream != stdin && fclose(stream) == EOF) {
            perror(argv[index]);
            status = EXIT_FAILURE;
        }
    }
    if (fflush(stdout) == EOF) {
        perror("stdout");
        status = EXIT_FAILURE;
    }
    return status;
}
