#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

static int wait_for_event(pid_t child, int *status, int options)
{
    pid_t waited;
    do {
        waited = waitpid(child, status, options);
    } while (waited == -1 && errno == EINTR);
    if (waited == -1) {
        perror("waitpid");
        return -1;
    }
    return 0;
}

static int terminate_job(pid_t *child)
{
    int status;
    if (*child <= 0)
        return 0;
    if (kill(*child, SIGKILL) == -1 && errno != ESRCH) {
        perror("kill");
        return -1;
    }
    if (wait_for_event(*child, &status, 0) == -1)
        return -1;
    *child = -1;
    return 0;
}

static pid_t start_job(int number, unsigned int seconds)
{
    int ready[2];
    char marker = 'R';
    ssize_t count;
    pid_t child;

    if (pipe(ready) == -1) {
        perror("pipe");
        return -1;
    }
    fflush(stdout);
    child = fork();
    if (child == -1) {
        perror("fork");
        close(ready[0]);
        close(ready[1]);
        return -1;
    }
    if (child == 0) {
        close(ready[0]);
        printf("START%d\n", number);
        fflush(stdout);
        do {
            count = write(ready[1], &marker, 1);
        } while (count == -1 && errno == EINTR);
        close(ready[1]);
        if (count != 1)
            _exit(1);
        while (seconds != 0)
            seconds = sleep(seconds);
        printf("DONE%d\n", number);
        fflush(stdout);
        _exit(0);
    }
    close(ready[1]);
    do {
        count = read(ready[0], &marker, 1);
    } while (count == -1 && errno == EINTR);
    close(ready[0]);
    if (count != 1) {
        fprintf(stderr, "Child startup failed\n");
        terminate_job(&child);
        return -1;
    }
    printf("[job %d] PID=%ld\n", number, (long)child);
    return child;
}

static int finish_job(pid_t *child)
{
    int status;
    if (wait_for_event(*child, &status, 0) == -1)
        return -1;
    *child = -1;
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        fprintf(stderr, "Job did not exit normally\n");
        return -1;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    unsigned int seconds = 10;
    pid_t first = -1;
    pid_t second = -1;
    pid_t demo = -1;
    int status;
    int result = EXIT_FAILURE;

    if (argc == 2) {
        char *end;
        errno = 0;
        long value = strtol(argv[1], &end, 10);
        if (errno != 0 || end == argv[1] || *end != '\0' || value < 1 || value > 60) {
            fprintf(stderr, "Usage: %s [seconds: 1-60, default 10]\n", argv[0]);
            return EXIT_FAILURE;
        }
        seconds = (unsigned int)value;
    } else if (argc != 1) {
        fprintf(stderr, "Usage: %s [seconds: 1-60, default 10]\n", argv[0]);
        return EXIT_FAILURE;
    }

    printf("(echo START1; sleep %u; echo DONE1) & 에 해당하는 작업 시작\n", seconds);
    first = start_job(1, seconds);
    if (first == -1)
        goto cleanup;
    printf("(echo START2; sleep %u; echo DONE2) & 에 해당하는 작업 시작\n", seconds);
    second = start_job(2, seconds);
    if (second == -1)
        goto cleanup;

    printf("\nkill -KILL %ld\n", (long)first);
    if (kill(first, SIGKILL) == -1) {
        perror("kill SIGKILL");
        goto cleanup;
    }
    if (wait_for_event(first, &status, 0) == -1)
        goto cleanup;
    first = -1;
    if (!WIFSIGNALED(status) || WTERMSIG(status) != SIGKILL) {
        fprintf(stderr, "First job was not terminated by SIGKILL\n");
        goto cleanup;
    }
    puts("첫 번째 작업은 SIGKILL로 종료되어 DONE1을 출력하지 않습니다.");
    puts("종료된 프로세스에는 STOP/CONT로 중지나 재개를 할 수 없습니다.");
    puts("별도 작업 3으로 STOP/CONT 동작을 확인합니다.");

    demo = start_job(3, seconds);
    if (demo == -1)
        goto cleanup;
    printf("\nkill -STOP %ld\n", (long)demo);
    if (kill(demo, SIGSTOP) == -1) {
        perror("kill SIGSTOP");
        goto cleanup;
    }
    if (wait_for_event(demo, &status, WUNTRACED) == -1)
        goto cleanup;
    if (!WIFSTOPPED(status)) {
        demo = -1;
        fprintf(stderr, "Demo job exited before stopping\n");
        goto cleanup;
    }
    printf("STOPPED: PID=%ld, signal=%d\n", (long)demo, WSTOPSIG(status));
    printf("kill -CONT %ld\n", (long)demo);
    if (kill(demo, SIGCONT) == -1) {
        perror("kill SIGCONT");
        goto cleanup;
    }
    if (wait_for_event(demo, &status, WCONTINUED) == -1)
        goto cleanup;
    if (!WIFCONTINUED(status)) {
        demo = -1;
        fprintf(stderr, "Demo job exited before observing continuation\n");
        goto cleanup;
    }
    printf("CONTINUED: PID=%ld\n", (long)demo);
    fflush(stdout);
    if (finish_job(&second) == -1 || finish_job(&demo) == -1)
        goto cleanup;
    puts("작업 2와 재개한 작업 3이 정상 종료되었습니다.");
    result = EXIT_SUCCESS;

cleanup:
    if (terminate_job(&first) == -1)
        result = EXIT_FAILURE;
    if (terminate_job(&second) == -1)
        result = EXIT_FAILURE;
    if (terminate_job(&demo) == -1)
        result = EXIT_FAILURE;
    return result;
}
