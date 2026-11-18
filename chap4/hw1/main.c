#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

int main(int argc, char *argv[])
{
    FILE *source;
    FILE *destination;
    struct stat source_info;
    struct stat destination_info;
    int character;
    int status = EXIT_SUCCESS;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s source destination\n", argv[0]);
        return EXIT_FAILURE;
    }
    source = fopen(argv[1], "r");
    if (source == NULL) {
        perror(argv[1]);
        return EXIT_FAILURE;
    }
    destination = fopen(argv[2], "a");
    if (destination == NULL) {
        perror(argv[2]);
        fclose(source);
        return EXIT_FAILURE;
    }
    if (fstat(fileno(source), &source_info) == -1 ||
        fstat(fileno(destination), &destination_info) == -1) {
        perror("fstat");
        status = EXIT_FAILURE;
    } else if (source_info.st_dev == destination_info.st_dev &&
               source_info.st_ino == destination_info.st_ino) {
        fprintf(stderr, "Source and destination must be different files.\n");
        status = EXIT_FAILURE;
    } else {
        while ((character = fgetc(source)) != EOF) {
            if (fputc(character, destination) == EOF) {
                perror("write");
                status = EXIT_FAILURE;
                break;
            }
        }
        if (ferror(source)) {
            perror("read");
            status = EXIT_FAILURE;
        }
    }
    if (fclose(source) == EOF) {
        perror("source close");
        status = EXIT_FAILURE;
    }
    if (fclose(destination) == EOF) {
        perror("destination close");
        status = EXIT_FAILURE;
    }
    return status;
}
