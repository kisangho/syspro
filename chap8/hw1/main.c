#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern char **environ;

static void usage(const char *program)
{
    fprintf(stderr, "Usage: %s -e[NAME] | -e [NAME] | -u | -g | -i | -p\n",
            program);
}

int main(int argc, char *argv[])
{
    int status = EXIT_SUCCESS;

    if (argc < 2) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }
    for (int index = 1; index < argc; index++) {
        if (strncmp(argv[index], "-e", 2) == 0) {
            const char *name = argv[index] + 2;
            if (*name == '\0' && index + 1 < argc && argv[index + 1][0] != '-')
                name = argv[++index];
            if (*name == '\0') {
                for (char **entry = environ; *entry != NULL; entry++)
                    puts(*entry);
            } else {
                const char *value = getenv(name);
                if (value == NULL) {
                    fprintf(stderr, "Environment variable not found: %s\n", name);
                    status = EXIT_FAILURE;
                } else {
                    printf("%s = %s\n", name, value);
                }
            }
        } else if (strcmp(argv[index], "-u") == 0) {
            printf("Real User ID: %lu\nEffective User ID: %lu\n",
                   (unsigned long)getuid(), (unsigned long)geteuid());
        } else if (strcmp(argv[index], "-g") == 0) {
            printf("Real Group ID: %lu\nEffective Group ID: %lu\n",
                   (unsigned long)getgid(), (unsigned long)getegid());
        } else if (strcmp(argv[index], "-i") == 0) {
            printf("My Process ID: %ld\n", (long)getpid());
        } else if (strcmp(argv[index], "-p") == 0) {
            printf("My Parent Process ID: %ld\n", (long)getppid());
        } else {
            usage(argv[0]);
            return EXIT_FAILURE;
        }
    }
    if (fflush(stdout) == EOF) {
        perror("stdout");
        status = EXIT_FAILURE;
    }
    return status;
}
