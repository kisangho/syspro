#include "../lines.h"

int main(int argc, char *argv[])
{
    struct text_lines text = {NULL, 0};
    int status = EXIT_SUCCESS;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s file\n", argv[0]);
        return EXIT_FAILURE;
    }
    if (read_lines(argv[1], &text) == -1) {
        perror(argv[1]);
        return EXIT_FAILURE;
    }
    for (size_t remaining = text.count; remaining > 0; remaining--) {
        if (puts(text.items[remaining - 1]) == EOF) {
            perror("stdout");
            status = EXIT_FAILURE;
            break;
        }
    }
    if (fflush(stdout) == EOF) {
        perror("stdout");
        status = EXIT_FAILURE;
    }
    free_lines(&text);
    return status;
}
