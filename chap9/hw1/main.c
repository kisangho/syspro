#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

static int wait_for_child(pid_t child, int *status)
{
    pid_t waited;
    do {
        waited = waitpid(child, status, 0);
    } while (waited == -1 && errno == EINTR);
    if (waited == -1) {
        perror("waitpid");
        return -1;
    }
    return 0;
}

static pid_t start_sleep(void)
{
    fflush(stdout);
    pid_t child = fork();
    if (child == 0) {
        execlp("sleep", "sleep", "100", (char *)NULL);
        perror("sleep");
        _exit(127);
    }
    if (child == -1)
        perror("fork");
    return child;
}

static int show_processes(void)
{
    int status;
    puts("\n$ ps -f");
    fflush(stdout);
    pid_t child = fork();
    if (child == -1) {
        perror("fork");
        return -1;
    }
    if (child == 0) {
        execlp("ps", "ps", "-f", (char *)NULL);
        perror("ps");
        _exit(127);
    }
    if (wait_for_child(child, &status) == -1)
        return -1;
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        fprintf(stderr, "ps -f failed\n");
        return -1;
    }
    return 0;
}

static int terminate_job(pid_t *child)
{
    int status;
    if (*child <= 0)
        return 0;
    if (kill(*child, SIGTERM) == -1 && errno != ESRCH) {
        perror("kill");
        return -1;
    }
    if (wait_for_child(*child, &status) == -1)
        return -1;
    *child = -1;
    return 0;
}

int main(void)
{
    pid_t first = -1;
    pid_t second = -1;
    int result = EXIT_FAILURE;

    puts("$ sleep 100 &");
    first = start_sleep();
    if (first == -1)
        goto cleanup;
    printf("[1] %ld\n", (long)first);
    puts("$ sleep 100 &");
    second = start_sleep();
    if (second == -1)
        goto cleanup;
    printf("[2] %ld\n", (long)second);
    puts("\n[jobs에 해당하는 C 프로그램의 작업 목록]");
    printf("[1]- Running  sleep 100  PID=%ld\n", (long)first);
    printf("[2]+ Running  sleep 100  PID=%ld\n", (long)second);
    puts("두 sleep은 기다리지 않고 시작했고, ps는 전면에서 종료를 기다립니다.");
    if (show_processes() == -1)
        goto cleanup;

    puts("\n$ kill %1  (첫 번째 작업의 PID에 SIGTERM 전달)");
    if (terminate_job(&first) == -1)
        goto cleanup;
    puts("첫 번째 작업 종료 및 회수 완료");
    if (show_processes() == -1)
        goto cleanup;
    puts("첫 번째 sleep은 종료되었고 두 번째 sleep은 계속 실행 중입니다.");
    result = EXIT_SUCCESS;

cleanup:
    if (terminate_job(&first) == -1)
        result = EXIT_FAILURE;
    if (second > 0)
        puts("실습 종료: 남은 두 번째 작업도 종료하고 회수합니다.");
    if (terminate_job(&second) == -1)
        result = EXIT_FAILURE;
    return result;
}
