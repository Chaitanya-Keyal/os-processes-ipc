/*
 * CS F372 Assignment 1, Problem 3
 *
 * logtop <file> <column>: the 5 most frequent values in a column of a log
 * file, with their counts. No counting or sorting is done here; the program
 * builds the pipeline
 *
 *     cut -d' ' -f<column> | sort | uniq -c | sort -rn | head -5
 *
 * with pipe/fork/dup2/exec, feeds it the file and copies the result to stdout.
 *
 * Build: gcc -Wall -o logtop logtop.c
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_LINE 512 /* longest log line, as given in the problem */

/*
 * Run stages[0] | stages[1] | ... | stages[n-1]. The first stage reads from
 * in_fd; the read end of a pipe attached to the last stage's stdout is
 * returned. Child pids are stored in pids[] so the caller can wait for them.
 */
static int run_pipeline(char* const* stages[], int n, int in_fd, pid_t pids[]) {
    int prev = in_fd;

    for (int i = 0; i < n; i++) {
        int fd[2];

        if (pipe(fd) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
        pids[i] = fork();
        if (pids[i] == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        if (pids[i] == 0) {
            dup2(prev, STDIN_FILENO);   /* read from the previous stage */
            dup2(fd[1], STDOUT_FILENO); /* write to the next one */
            close(prev);
            close(fd[0]);
            close(fd[1]);
            execvp(stages[i][0], stages[i]);
            perror(stages[i][0]);
            _exit(127);
        }
        close(prev);
        close(fd[1]);
        prev = fd[0];
    }
    return prev;
}

static void copy_to_stdout(int fd) {
    char buf[MAX_LINE];
    ssize_t n;

    while ((n = read(fd, buf, sizeof buf)) > 0)
        if (write(STDOUT_FILENO, buf, n) != n) break;
}

int main(int argc, char* argv[]) {
    char field[32];
    char* end;
    long column;
    int file, out, status = EXIT_SUCCESS;

    if (argc != 3) {
        fprintf(stderr, "usage: %s <logfile> <column>\n", argv[0]);
        return EXIT_FAILURE;
    }
    column = strtol(argv[2], &end, 10);
    if (*end != '\0' || column < 1) {
        fprintf(stderr, "%s: column must be a positive integer\n", argv[0]);
        return EXIT_FAILURE;
    }
    file = open(argv[1], O_RDONLY);
    if (file == -1) {
        fprintf(stderr, "%s: %s: %s\n", argv[0], argv[1], strerror(errno));
        return EXIT_FAILURE;
    }

    snprintf(field, sizeof field, "-f%ld", column);
    char* cut[] = {"cut", "-d", " ", field, NULL};
    char* sort[] = {"sort", NULL};
    char* uniq[] = {"uniq", "-c", NULL};
    char* sort_rn[] = {"sort", "-rn", NULL};
    char* head[] = {"head", "-5", NULL};
    char* const* stages[] = {cut, sort, uniq, sort_rn, head};
    pid_t pids[5];

    out = run_pipeline(stages, 5, file, pids);
    copy_to_stdout(out);

    for (int i = 0; i < 5; i++) {
        int st;
        waitpid(pids[i], &st, 0);
        if (!WIFEXITED(st) || WEXITSTATUS(st) != 0) status = EXIT_FAILURE;
    }
    return status;
}
