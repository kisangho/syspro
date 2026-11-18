#ifndef SYSPRO_RECORD_IO_H
#define SYSPRO_RECORD_IO_H

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "prob1/student.h"

static int input_integer(const char *prompt, int *value)
{
    char *line = NULL;
    char *end;
    size_t capacity = 0;
    long parsed;

    fputs(prompt, stdout);
    fflush(stdout);
    if (getline(&line, &capacity, stdin) == -1) {
        free(line);
        return 0;
    }
    errno = 0;
    parsed = strtol(line, &end, 10);
    if (end == line || errno != 0 || parsed < INT_MIN || parsed > INT_MAX) {
        free(line);
        return -1;
    }
    end += strspn(end, " \t\r\n");
    if (*end != '\0') {
        free(line);
        return -1;
    }
    *value = (int)parsed;
    free(line);
    return 1;
}

static int continue_records(void)
{
    char *line = NULL;
    size_t capacity = 0;
    int again;

    fputs("Continue? (Y/N): ", stdout);
    fflush(stdout);
    again = getline(&line, &capacity, stdin) != -1 &&
            (line[0] == 'Y' || line[0] == 'y');
    free(line);
    return again;
}

static int record_lock(int descriptor, off_t offset, short type, int use_lockf)
{
    int result;
    if (use_lockf) {
        if (lseek(descriptor, offset, SEEK_SET) == -1)
            return -1;
        do {
            result = lockf(descriptor, type == F_UNLCK ? F_ULOCK : F_LOCK,
                           (off_t)sizeof(struct student));
        } while (result == -1 && errno == EINTR);
    } else {
        struct flock lock = {0};
        lock.l_type = type;
        lock.l_whence = SEEK_SET;
        lock.l_start = offset;
        lock.l_len = sizeof(struct student);
        do {
            result = fcntl(descriptor, type == F_UNLCK ? F_SETLK : F_SETLKW,
                           &lock);
        } while (result == -1 && errno == EINTR);
    }
    return result;
}

static int run_records(int argc, char *argv[], int writable, int use_lockf)
{
    int descriptor;
    int status = EXIT_SUCCESS;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s test.txt\n", argv[0]);
        return EXIT_FAILURE;
    }
    descriptor = open(argv[1], writable ? O_RDWR : O_RDONLY);
    if (descriptor == -1) {
        perror(argv[1]);
        return EXIT_FAILURE;
    }
    do {
        struct student record;
        int student_id;
        int input = input_integer(writable ? "Enter StudentID to modify: "
                                            : "Enter StudentID to search: ",
                                  &student_id);
        if (input == 0)
            break;
        if (input == -1 || student_id < START_ID) {
            fprintf(stderr, "Invalid StudentID (minimum %d).\n", START_ID);
            continue;
        }
        off_t offset = (off_t)(student_id - START_ID) * sizeof(record);
        if (record_lock(descriptor, offset, writable ? F_WRLCK : F_RDLCK,
                        use_lockf) == -1) {
            perror(argv[1]);
            status = EXIT_FAILURE;
            break;
        }
        ssize_t count;
        do {
            count = pread(descriptor, &record, sizeof(record), offset);
        } while (count == -1 && errno == EINTR);
        if (count == -1) {
            perror("pread");
            status = EXIT_FAILURE;
        } else if (count != sizeof(record) || record.id != student_id) {
            printf("Record %d not found.\n", student_id);
        } else {
            printf("Name:%.*s\t StuID:%d\t Score:%d\n",
                   MAX, record.name, record.id, record.score);
            if (writable) {
                input = input_integer("Enter new score: ", &record.score);
                if (input == -1) {
                    fprintf(stderr, "Invalid score.\n");
                } else if (input == 1) {
                    size_t written = 0;
                    while (written < sizeof(record)) {
                        count = pwrite(descriptor, (char *)&record + written,
                                       sizeof(record) - written, offset + written);
                        if (count == -1 && errno == EINTR)
                            continue;
                        if (count <= 0) {
                            if (count == 0)
                                errno = EIO;
                            perror("pwrite");
                            status = EXIT_FAILURE;
                            break;
                        }
                        written += (size_t)count;
                    }
                    if (status == EXIT_SUCCESS)
                        puts("Record updated.");
                }
            }
        }
        if (record_lock(descriptor, offset, F_UNLCK, use_lockf) == -1) {
            perror("unlock");
            status = EXIT_FAILURE;
        }
        if (status != EXIT_SUCCESS || input == 0)
            break;
    } while (continue_records());
    if (close(descriptor) == -1) {
        perror("close");
        status = EXIT_FAILURE;
    }
    return status;
}

#endif
