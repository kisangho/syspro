#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include "../process.h"

#define MAXARG 128

int main(void)
{
    char *line = NULL;
    size_t capacity = 0;
    int status = 0;

    if (initialize_shell() == -1) {
        perror("sigaction");
        return EXIT_FAILURE;
    }
    setvbuf(stdin, NULL, _IONBF, 0);
    for (;;) {
        char *arguments[MAXARG];
        char *saved = NULL;
        char *argument;
        size_t count = 0;
        int background = 0;
        int invalid = 0;

        if (isatty(STDIN_FILENO)) {
            fputs("[Shell] ", stdout);
            fflush(stdout);
        }
        if (getline(&line, &capacity, stdin) == -1)
            break;
        size_t length = strlen(line);
        while (length > 0 && strchr(" \t\r\n", line[length - 1]) != NULL)
            line[--length] = '\0';
        if (length > 0 && line[length - 1] == '&') {
            background = 1;
            line[--length] = '\0';
        }
        if (strpbrk(line, "&;<>") != NULL) {
            fprintf(stderr, "Only a command and an optional trailing & are supported.\n");
            status = 2;
            continue;
        }
        argument = strtok_r(line, " \t\r\n", &saved);
        while (argument != NULL) {
            if (count == MAXARG - 1) {
                fprintf(stderr, "Too many arguments (maximum %d).\n", MAXARG - 1);
                invalid = 1;
                break;
            }
            arguments[count++] = argument;
            argument = strtok_r(NULL, " \t\r\n", &saved);
        }
        if (invalid) {
            status = 2;
            continue;
        }
        if (count == 0) {
            if (background) {
                fprintf(stderr, "Missing command before &.\n");
                status = 2;
            }
            continue;
        }
        arguments[count] = NULL;
        if (strcmp(arguments[0], "exit") == 0) {
            if (count != 1 || background) {
                fprintf(stderr, "Usage: exit\n");
                status = 2;
                continue;
            }
            status = 0;
            break;
        }
        status = execute_command(arguments, NULL, NULL, background);
    }
    if (ferror(stdin)) {
        perror("stdin");
        status = 1;
    }
    free(line);
    return status;
}
