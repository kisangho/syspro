#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <dirent.h>
#include <errno.h>
#include <grp.h>
#include <inttypes.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

static char file_type(mode_t mode)
{
    if (S_ISREG(mode)) return '-';
    if (S_ISDIR(mode)) return 'd';
    if (S_ISCHR(mode)) return 'c';
    if (S_ISBLK(mode)) return 'b';
    if (S_ISLNK(mode)) return 'l';
    if (S_ISFIFO(mode)) return 'p';
    if (S_ISSOCK(mode)) return 's';
    return '?';
}

static void print_entry(const char *name, const struct stat *info,
                        int inode, int slash, int quote)
{
    const mode_t bits[] = {S_IRUSR, S_IWUSR, S_IXUSR, S_IRGRP, S_IWGRP,
                          S_IXGRP, S_IROTH, S_IWOTH, S_IXOTH};
    const char symbols[] = "rwxrwxrwx";
    char permissions[10] = "---------";
    char timestamp[32] = "unknown-time";
    struct passwd *owner;
    struct group *group;
    struct tm *modified = localtime(&info->st_mtime);

    for (size_t index = 0; index < 9; index++)
        if (info->st_mode & bits[index])
            permissions[index] = symbols[index];
    if (info->st_mode & S_ISUID)
        permissions[2] = (info->st_mode & S_IXUSR) ? 's' : 'S';
    if (info->st_mode & S_ISGID)
        permissions[5] = (info->st_mode & S_IXGRP) ? 's' : 'S';
    if (info->st_mode & S_ISVTX)
        permissions[8] = (info->st_mode & S_IXOTH) ? 't' : 'T';
    if (modified != NULL)
        strftime(timestamp, sizeof(timestamp), "%b %d %H:%M", modified);
    if (inode)
        printf("%ju ", (uintmax_t)info->st_ino);
    printf("%5jd %c%s %3ju ", (intmax_t)info->st_blocks,
           file_type(info->st_mode), permissions, (uintmax_t)info->st_nlink);
    owner = getpwuid(info->st_uid);
    if (owner != NULL)
        printf("%s ", owner->pw_name);
    else
        printf("%ju ", (uintmax_t)info->st_uid);
    group = getgrgid(info->st_gid);
    if (group != NULL)
        printf("%s ", group->gr_name);
    else
        printf("%ju ", (uintmax_t)info->st_gid);
    printf("%9jd %s ", (intmax_t)info->st_size, timestamp);
    if (quote)
        putchar('"');
    for (const char *cursor = name; *cursor != '\0'; cursor++) {
        if (quote && (*cursor == '"' || *cursor == '\\'))
            putchar('\\');
        if (quote && *cursor == '\n')
            fputs("\\n", stdout);
        else if (quote && *cursor == '\t')
            fputs("\\t", stdout);
        else
            putchar(*cursor);
    }
    if (quote)
        putchar('"');
    if (slash && S_ISDIR(info->st_mode))
        putchar('/');
    putchar('\n');
}

int main(int argc, char *argv[])
{
    int inode = 0;
    int slash = 0;
    int quote = 0;
    int option;
    int status = EXIT_SUCCESS;
    DIR *directory;
    const char *path;

    while ((option = getopt(argc, argv, "ipQ")) != -1) {
        switch (option) {
        case 'i': inode = 1; break;
        case 'p': slash = 1; break;
        case 'Q': quote = 1; break;
        default:
            fprintf(stderr, "Usage: %s [-ipQ] [directory]\n", argv[0]);
            return EXIT_FAILURE;
        }
    }
    if (argc - optind > 1) {
        fprintf(stderr, "Usage: %s [-ipQ] [directory]\n", argv[0]);
        return EXIT_FAILURE;
    }
    path = optind < argc ? argv[optind] : ".";
    directory = opendir(path);
    if (directory == NULL) {
        perror(path);
        return EXIT_FAILURE;
    }
    for (;;) {
        struct dirent *entry;
        struct stat info;
        errno = 0;
        entry = readdir(directory);
        if (entry == NULL) {
            if (errno != 0) {
                perror("readdir");
                status = EXIT_FAILURE;
            }
            break;
        }
        size_t length = strlen(path) + strlen(entry->d_name) + 2;
        char *full_path = malloc(length);
        if (full_path == NULL) {
            perror("malloc");
            status = EXIT_FAILURE;
            break;
        }
        snprintf(full_path, length, "%s/%s", path, entry->d_name);
        if (lstat(full_path, &info) == -1) {
            perror(full_path);
            status = EXIT_FAILURE;
        } else {
            print_entry(entry->d_name, &info, inode, slash, quote);
        }
        free(full_path);
    }
    if (closedir(directory) == -1 || fflush(stdout) == EOF) {
        perror("close/output");
        status = EXIT_FAILURE;
    }
    return status;
}
