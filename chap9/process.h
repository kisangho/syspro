#ifndef SYSPRO_PROCESS_H
#define SYSPRO_PROCESS_H

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static void reap_children(int signal_number)
{
    int saved_errno = errno;
    (void)signal_number;
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
    errno = saved_errno;
}

static int initialize_shell(void)
{
    struct sigaction action = {0};
    action.sa_handler = reap_children;
    action.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGCHLD, &action, NULL) == -1)
        return -1;
    action.sa_handler = SIG_IGN;
    if (sigaction(SIGINT, &action, NULL) == -1 ||
        sigaction(SIGQUIT, &action, NULL) == -1)
        return -1;
    return 0;
}

static int redirect_file(const char *path, int target, int flags)
{
    int descriptor = open(path, flags, 0666);
    if (descriptor == -1) {
        perror(path);
        return -1;
    }
    if (descriptor != target) {
        if (dup2(descriptor, target) == -1) {
            perror("dup2");
            close(descriptor);
            return -1;
        }
        if (close(descriptor) == -1) {
            perror("close");
            return -1;
        }
    }
    return 0;
}

static int execute_command(char *arguments[], const char *input,
                           const char *output, int background)
{
    sigset_t blocked;
    sigset_t previous;
    pid_t child;
    int status;

    sigemptyset(&blocked);
    sigaddset(&blocked, SIGCHLD);
    if (sigprocmask(SIG_BLOCK, &blocked, &previous) == -1) {
        perror("sigprocmask");
        return 1;
    }
    fflush(NULL);
    child = fork();
    if (child == -1) {
        perror("fork");
        sigprocmask(SIG_SETMASK, &previous, NULL);
        return 1;
    }
    if (child == 0) {
        struct sigaction action = {0};
        action.sa_handler = SIG_DFL;
        sigemptyset(&action.sa_mask);
        if (sigaction(SIGINT, &action, NULL) == -1 ||
            sigaction(SIGQUIT, &action, NULL) == -1 ||
            sigaction(SIGCHLD, &action, NULL) == -1 ||
            sigprocmask(SIG_SETMASK, &previous, NULL) == -1) {
            perror("signal setup");
            _exit(126);
        }
        if (background && setpgid(0, 0) == -1) {
            perror("setpgid");
            _exit(126);
        }
        if (input == NULL && background)
            input = "/dev/null";
        if (input != NULL && redirect_file(input, STDIN_FILENO, O_RDONLY) == -1)
            _exit(126);
        if (output != NULL && redirect_file(output, STDOUT_FILENO,
                                            O_WRONLY | O_CREAT | O_TRUNC) == -1)
            _exit(126);
        execvp(arguments[0], arguments);
        int saved_errno = errno;
        perror(arguments[0]);
        _exit(saved_errno == ENOENT ? 127 : 126);
    }
    if (background) {
        printf("[background] %ld\n", (long)child);
        fflush(stdout);
        sigprocmask(SIG_SETMASK, &previous, NULL);
        return 0;
    }
    pid_t waited;
    do {
        waited = waitpid(child, &status, 0);
    } while (waited == -1 && errno == EINTR);
    int saved_errno = errno;
    sigprocmask(SIG_SETMASK, &previous, NULL);
    if (waited == -1) {
        errno = saved_errno;
        perror("waitpid");
        return 1;
    }
    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    if (WIFSIGNALED(status))
        return 128 + WTERMSIG(status);
    return 1;
}

#endif
