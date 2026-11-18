#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <string.h>
#include "../lines.h"

static void skip_spaces(char **cursor)
{
    while (isspace((unsigned char)**cursor))
        (*cursor)++;
}

static int parse_number(char **cursor, size_t maximum, size_t *number)
{
    char *end;
    unsigned long long value;

    skip_spaces(cursor);
    if (!isdigit((unsigned char)**cursor))
        return -1;
    errno = 0;
    value = strtoull(*cursor, &end, 10);
    if (errno != 0 || value == 0 || value > maximum)
        return -1;
    *cursor = end;
    *number = (size_t)value;
    return 0;
}

static int select_lines(char *request, const struct text_lines *text, int print)
{
    char *cursor = request;
    size_t first;
    size_t last;

    skip_spaces(&cursor);
    if (*cursor == '*') {
        cursor++;
        skip_spaces(&cursor);
        if (*cursor != '\0')
            return -1;
        if (print)
            for (size_t index = 0; index < text->count; index++)
                printf("%zu: %s\n", index + 1, text->items[index]);
        return 0;
    }
    for (;;) {
        if (parse_number(&cursor, text->count, &first) == -1)
            return -1;
        skip_spaces(&cursor);
        last = first;
        if (*cursor == '-') {
            cursor++;
            if (parse_number(&cursor, text->count, &last) == -1 || last < first)
                return -1;
            skip_spaces(&cursor);
        }
        if (print)
            for (size_t number = first; number <= last; number++)
                printf("%zu: %s\n", number, text->items[number - 1]);
        if (*cursor == '\0')
            return 0;
        if (*cursor++ != ',')
            return -1;
    }
}

int main(int argc, char *argv[])
{
    struct text_lines text = {NULL, 0};
    char *request = NULL;
    size_t capacity = 0;
    int status = EXIT_SUCCESS;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s file\n", argv[0]);
        return EXIT_FAILURE;
    }
    if (read_lines(argv[1], &text) == -1) {
        perror(argv[1]);
        return EXIT_FAILURE;
    }
    printf("Total Line: %zu\n", text.count);
    for (;;) {
        fputs("Input (n, n,m, n-m, *, q): ", stdout);
        fflush(stdout);
        if (getline(&request, &capacity, stdin) == -1)
            break;
        request[strcspn(request, "\r\n")] = '\0';
        if (strcmp(request, "q") == 0)
            break;
        if (select_lines(request, &text, 0) == -1) {
            fprintf(stderr, "Invalid line selection (1-%zu).\n", text.count);
            continue;
        }
        select_lines(request, &text, 1);
    }
    if (ferror(stdin) || fflush(stdout) == EOF) {
        perror("input/output");
        status = EXIT_FAILURE;
    }
    free(request);
    free_lines(&text);
    return status;
}
