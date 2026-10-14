#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <grp.h>
#include <pwd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <errno.h>

char type(mode_t mode);
char *perm(mode_t mode);
void printStat(const char *file, const struct stat *st);

int main(int argc, char **argv)
{
    DIR *dp;
    const char *dir;
    struct stat st;
    struct dirent *d;
    char path[BUFSIZ + 1];
    int status = EXIT_SUCCESS;

    dir = (argc == 1) ? "." : argv[1];

    dp = opendir(dir);
    if (dp == NULL) {
        perror(dir);
        return EXIT_FAILURE;
    }

    for (;;) {
        errno = 0;
        d = readdir(dp);

        if (d == NULL) {
            if (errno != 0) {
                perror("readdir");
                status = EXIT_FAILURE;
            }
            break;
        }

        int n = snprintf(path, sizeof(path), "%s/%s",
                         dir, d->d_name);

        if (n < 0 || (size_t)n >= sizeof(path)) {
            fprintf(stderr, "Path too long: %s/%s\n",
                    dir, d->d_name);
            status = EXIT_FAILURE;
            continue;
        }

        if (lstat(path, &st) < 0) {
            perror(path);
            status = EXIT_FAILURE;
            continue;
        }

        printStat(d->d_name, &st);
    }

    if (closedir(dp) < 0) {
        perror("closedir");
        status = EXIT_FAILURE;
    }

    return status;
}

void printStat(const char *file, const struct stat *st)
{
    struct passwd *pw;
    struct group *gr;
    char timebuf[32];
    struct tm *tm_info;
    time_t mtime = st->st_mtime;

    printf("%5jd ", (intmax_t)st->st_blocks);
    printf("%c%s ", type(st->st_mode), perm(st->st_mode));
    printf("%3ju ", (uintmax_t)st->st_nlink);

    pw = getpwuid(st->st_uid);
    gr = getgrgid(st->st_gid);

    if (pw != NULL)
        printf("%s ", pw->pw_name);
    else
        printf("%ju ", (uintmax_t)st->st_uid);

    if (gr != NULL)
        printf("%s ", gr->gr_name);
    else
        printf("%ju ", (uintmax_t)st->st_gid);

    printf("%9jd ", (intmax_t)st->st_size);

    tm_info = localtime(&mtime);

    if (tm_info != NULL &&
        strftime(timebuf, sizeof(timebuf),
                 "%b %d %H:%M", tm_info) > 0) {
        printf("%s ", timebuf);
    } else {
        printf("unknown-time ");
    }

    printf("%s\n", file);
}

char type(mode_t mode)
{
    if (S_ISREG(mode))  return '-';
    if (S_ISDIR(mode))  return 'd';
    if (S_ISCHR(mode))  return 'c';
    if (S_ISBLK(mode))  return 'b';
    if (S_ISLNK(mode))  return 'l';
    if (S_ISFIFO(mode)) return 'p';
    if (S_ISSOCK(mode)) return 's';

    return '?';
}

char *perm(mode_t mode)
{
    static char perms[10] = "---------";

    /* Owner permissions */
    if (mode & S_IRUSR) perms[0] = 'r';
    else                perms[0] = '-';

    if (mode & S_IWUSR) perms[1] = 'w';
    else                perms[1] = '-';

    if (mode & S_IXUSR) perms[2] = 'x';
    else                perms[2] = '-';

    /* Group permissions */
    if (mode & S_IRGRP) perms[3] = 'r';
    else                perms[3] = '-';

    if (mode & S_IWGRP) perms[4] = 'w';
    else                perms[4] = '-';

    if (mode & S_IXGRP) perms[5] = 'x';
    else                perms[5] = '-';

    /* Other permissions */
    if (mode & S_IROTH) perms[6] = 'r';
    else                perms[6] = '-';

    if (mode & S_IWOTH) perms[7] = 'w';
    else                perms[7] = '-';

    if (mode & S_IXOTH) perms[8] = 'x';
    else                perms[8] = '-';

    /* Special permission bits */
    if (mode & S_ISUID)
        perms[2] = (mode & S_IXUSR) ? 's' : 'S';

    if (mode & S_ISGID)
        perms[5] = (mode & S_IXGRP) ? 's' : 'S';

    if (mode & S_ISVTX)
        perms[8] = (mode & S_IXOTH) ? 't' : 'T';

    return perms;
}
