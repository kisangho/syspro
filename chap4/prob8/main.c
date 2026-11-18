#include <stdio.h>
#include <stdio_ext.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    FILE *stream;
    const char *mode;
    int is_file = 0;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s stdin|stdout|stderr|file\n", argv[0]);
        return EXIT_FAILURE;
    }
    if (strcmp(argv[1], "stdin") == 0) {
        stream = stdin;
        fputs("Enter one letter: ", stdout);
        fflush(stdout);
        if (fgetc(stream) == EOF && ferror(stream)) {
            perror("stdin");
            return EXIT_FAILURE;
        }
    } else if (strcmp(argv[1], "stdout") == 0) {
        stream = stdout;
        fputs("stdout test\n", stream);
    } else if (strcmp(argv[1], "stderr") == 0) {
        stream = stderr;
        fputs("stderr test\n", stream);
    } else {
        stream = fopen(argv[1], "r");
        if (stream == NULL) {
            perror(argv[1]);
            return EXIT_FAILURE;
        }
        is_file = 1;
        if (fgetc(stream) == EOF && ferror(stream)) {
            perror(argv[1]);
            fclose(stream);
            return EXIT_FAILURE;
        }
    }

    if (__flbf(stream))
        mode = "Line buffered";
    else if (__fbufsize(stream) <= 1)
        mode = "Unbuffered";
    else
        mode = "Fully buffered";
    printf("Stream = %s, %s, Buffer size = %zu\n",
           argv[1], mode, __fbufsize(stream));
    if (is_file && fclose(stream) == EOF) {
        perror("fclose");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
