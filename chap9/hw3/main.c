#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <string.h>
#include "../process.h"

#define MAXARG 128

enum token_type { TOKEN_END, TOKEN_WORD, TOKEN_SEMICOLON, TOKEN_BACKGROUND,
                  TOKEN_INPUT, TOKEN_OUTPUT, TOKEN_ERROR };

struct token {
    enum token_type type;
    char *word;
};

struct command {
    char *arguments[MAXARG];
    size_t count;
    char *input;
    char *output;
    int background;
};

static struct token next_token(char **cursor)
{
    struct token token = {TOKEN_END, NULL};
    char *word;
    size_t length = 0;
    char quote = '\0';

    while (isspace((unsigned char)**cursor))
        (*cursor)++;
    switch (**cursor) {
    case '\0': return token;
    case ';': token.type = TOKEN_SEMICOLON; break;
    case '&': token.type = TOKEN_BACKGROUND; break;
    case '<': token.type = TOKEN_INPUT; break;
    case '>': token.type = TOKEN_OUTPUT; break;
    default: token.type = TOKEN_WORD; break;
    }
    if (token.type != TOKEN_WORD) {
        (*cursor)++;
        return token;
    }
    word = malloc(strlen(*cursor) + 1);
    if (word == NULL) {
        perror("malloc");
        token.type = TOKEN_ERROR;
        return token;
    }
    while (**cursor != '\0') {
        char character = **cursor;
        if (quote == '\0' && (isspace((unsigned char)character) ||
                              strchr(";&<>", character) != NULL))
            break;
        (*cursor)++;
        if (character == quote) {
            quote = '\0';
        } else if (quote == '\0' && (character == '\'' || character == '"')) {
            quote = character;
        } else if (character == '\\' && quote != '\'') {
            if (**cursor == '\0') {
                fprintf(stderr, "Incomplete escape.\n");
                free(word);
                token.type = TOKEN_ERROR;
                return token;
            }
            if (quote == '"' && **cursor != '"' && **cursor != '\\' &&
                **cursor != '$' && **cursor != '`')
                word[length++] = '\\';
            word[length++] = *(*cursor)++;
        } else {
            word[length++] = character;
        }
    }
    if (quote != '\0') {
        fprintf(stderr, "Unclosed quote.\n");
        free(word);
        token.type = TOKEN_ERROR;
        return token;
    }
    word[length] = '\0';
    token.word = word;
    return token;
}

static void free_command(struct command *command)
{
    for (size_t index = 0; index < command->count; index++)
        free(command->arguments[index]);
    free(command->input);
    free(command->output);
    memset(command, 0, sizeof(*command));
}

static int run_command(struct command *command, int *exiting)
{
    if (command->count == 0) {
        fprintf(stderr, "Missing command.\n");
        return 2;
    }
    if (strcmp(command->arguments[0], "exit") == 0) {
        if (command->count != 1 || command->background || command->input ||
            command->output) {
            fprintf(stderr, "Usage: exit\n");
            return 2;
        }
        *exiting = 1;
        return 0;
    }
    return execute_command(command->arguments, command->input, command->output,
                           command->background);
}

static int run_line(char *line, int *exiting)
{
    struct command command = {0};
    char *cursor = line;
    int status = 0;
    int after_background = 0;

    for (;;) {
        struct token token = next_token(&cursor);
        if (token.type == TOKEN_ERROR) {
            status = 2;
            break;
        }
        if (token.type == TOKEN_WORD) {
            if (command.count == MAXARG - 1) {
                fprintf(stderr, "Too many arguments (maximum %d).\n", MAXARG - 1);
                free(token.word);
                status = 2;
                break;
            }
            command.arguments[command.count++] = token.word;
            command.arguments[command.count] = NULL;
            after_background = 0;
        } else if (token.type == TOKEN_INPUT || token.type == TOKEN_OUTPUT) {
            struct token filename = next_token(&cursor);
            char **target = token.type == TOKEN_INPUT ? &command.input
                                                      : &command.output;
            if (filename.type != TOKEN_WORD || *target != NULL) {
                fprintf(stderr, "Expected one filename per redirection.\n");
                free(filename.word);
                status = 2;
                break;
            }
            *target = filename.word;
            after_background = 0;
        } else {
            if (token.type == TOKEN_END && command.count == 0 &&
                command.input == NULL && command.output == NULL)
                break;
            if (token.type == TOKEN_BACKGROUND && command.count == 0) {
                fprintf(stderr, "Missing command before &: logical && is unsupported.\n");
                status = 2;
                break;
            }
            if (token.type == TOKEN_SEMICOLON && command.count == 0 &&
                after_background) {
                after_background = 0;
                continue;
            }
            command.background = token.type == TOKEN_BACKGROUND;
            status = run_command(&command, exiting);
            free_command(&command);
            after_background = token.type == TOKEN_BACKGROUND;
            if (*exiting || token.type == TOKEN_END)
                break;
        }
    }
    free_command(&command);
    return status;
}

int main(void)
{
    char *line = NULL;
    size_t capacity = 0;
    int exiting = 0;
    int status = 0;

    if (initialize_shell() == -1) {
        perror("sigaction");
        return EXIT_FAILURE;
    }
    setvbuf(stdin, NULL, _IONBF, 0);
    while (!exiting) {
        if (isatty(STDIN_FILENO)) {
            fputs("[shell] ", stdout);
            fflush(stdout);
        }
        if (getline(&line, &capacity, stdin) == -1)
            break;
        status = run_line(line, &exiting);
    }
    if (ferror(stdin)) {
        perror("stdin");
        status = 1;
    }
    free(line);
    return status;
}
