#ifndef SYSPRO_LINES_H
#define SYSPRO_LINES_H

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct text_lines {
    char **items;
    size_t count;
};

static void free_lines(struct text_lines *text)
{
    for (size_t index = 0; index < text->count; index++)
        free(text->items[index]);
    free(text->items);
    text->items = NULL;
    text->count = 0;
}

static int append_line(struct text_lines *text, char *line)
{
    if (text->count >= SIZE_MAX / sizeof(*text->items)) {
        errno = ENOMEM;
        return -1;
    }
    char **items = realloc(text->items, (text->count + 1) * sizeof(*items));
    if (items == NULL)
        return -1;
    text->items = items;
    text->items[text->count++] = line;
    return 0;
}

static int read_lines(const char *path, struct text_lines *text)
{
    int descriptor = open(path, O_RDONLY);
    char *line = NULL;
    size_t length = 0;
    size_t capacity = 0;
    char character;
    ssize_t count;

    if (descriptor == -1)
        return -1;
    for (;;) {
        count = read(descriptor, &character, 1);
        if (count == -1 && errno == EINTR)
            continue;
        if (count == -1)
            goto failure;
        if (count == 0)
            break;
        if (length + 1 >= capacity) {
            if (capacity > SIZE_MAX / 2) {
                errno = ENOMEM;
                goto failure;
            }
            size_t new_capacity = capacity == 0 ? 128 : capacity * 2;
            char *expanded = realloc(line, new_capacity);
            if (expanded == NULL)
                goto failure;
            line = expanded;
            capacity = new_capacity;
        }
        if (character == '\n') {
            line[length] = '\0';
            if (append_line(text, line) == -1)
                goto failure;
            line = NULL;
            length = 0;
            capacity = 0;
        } else {
            line[length++] = character;
        }
    }
    if (length != 0) {
        line[length] = '\0';
        if (append_line(text, line) == -1)
            goto failure;
        line = NULL;
    }
    free(line);
    if (close(descriptor) == -1) {
        free_lines(text);
        return -1;
    }
    return 0;

failure:
    {
        int saved_errno = errno;
        free(line);
        free_lines(text);
        close(descriptor);
        errno = saved_errno;
        return -1;
    }
}

#endif
