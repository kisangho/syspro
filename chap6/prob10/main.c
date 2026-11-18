#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

static int prepare_file(const char *path)
{
    const char content[] = "System programming: file status practice.\n";
    int descriptor = open(path, O_WRONLY | O_CREAT | O_EXCL, 0644);
    size_t written = 0;

    if (descriptor == -1) {
        struct stat info;
        if (errno != EEXIST || stat(path, &info) == -1) {
            perror(path);
            return -1;
        }
        if (!S_ISREG(info.st_mode)) {
            fprintf(stderr, "%s: regular file required\n", path);
            return -1;
        }
        return 0;
    }
    while (written < sizeof(content) - 1) {
        ssize_t count = write(descriptor, content + written,
                              sizeof(content) - 1 - written);
        if (count == -1 && errno == EINTR)
            continue;
        if (count <= 0) {
            if (count == 0)
                errno = EIO;
            perror("write");
            close(descriptor);
            return -1;
        }
        written += (size_t)count;
    }
    if (close(descriptor) == -1) {
        perror("close");
        return -1;
    }
    return 0;
}

static int run_command(char *arguments[])
{
    int status;
    pid_t child;
    pid_t waited;

    fflush(stdout);
    child = fork();
    if (child == -1) {
        perror("fork");
        return -1;
    }
    if (child == 0) {
        execvp(arguments[0], arguments);
        perror(arguments[0]);
        _exit(127);
    }
    do {
        waited = waitpid(child, &status, 0);
    } while (waited == -1 && errno == EINTR);
    if (waited == -1) {
        perror("waitpid");
        return -1;
    }
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        fprintf(stderr, "%s failed\n", arguments[0]);
        return -1;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    char *path = argc == 2 ? argv[1] : "test.txt";
    char *list_arguments[] = {"ls", "-sl", "--", path, NULL};
    char *stat_arguments[] = {"stat", "--", path, NULL};
    char *inode_arguments[] = {"ls", "-i", "--", path, NULL};
    char *touch_arguments[] = {"touch", "--", path, NULL};

    if (argc > 2) {
        fprintf(stderr, "Usage: %s [test.txt]\n", argv[0]);
        return EXIT_FAILURE;
    }
    if (prepare_file(path) == -1)
        return EXIT_FAILURE;

    puts("[ls -sl] 할당된 블록 용량, 권한, 링크 수, 소유자, 그룹, 크기, 수정 시각");
    if (run_command(list_arguments) == -1)
        return EXIT_FAILURE;
    puts("\n[stat: before touch] 파일 속성과 접근/수정/상태 변경 시각");
    if (run_command(stat_arguments) == -1)
        return EXIT_FAILURE;
    puts("\n[ls -i] 파일의 inode 번호");
    if (run_command(inode_arguments) == -1)
        return EXIT_FAILURE;
    unsigned int remaining = 1;
    while (remaining != 0)
        remaining = sleep(remaining);
    puts("\n[touch] 접근 시각과 수정 시각을 현재 시각으로 변경");
    if (run_command(touch_arguments) == -1)
        return EXIT_FAILURE;
    puts("\n[stat: after touch] 앞의 stat 출력과 비교");
    if (run_command(stat_arguments) == -1)
        return EXIT_FAILURE;
    puts("파일 내용, 크기, inode, 권한은 유지되고 atime, mtime, ctime이 갱신됩니다.");
    puts("ctime은 생성 시각이 아니라 파일 상태 변경 시각입니다.");
    return EXIT_SUCCESS;
}
