/*
 * CS F372 Assignment 1, Problem 2: weighted resource monitor
 *
 *     usage_score = 3 * CPU% + 2 * MEM%
 *
 * The child prints the top k processes every n seconds using
 *
 *     ps axo pid=,%cpu=,%mem=,comm= | awk '...' | sort -rn | head
 *
 * built with pipe/fork/dup2/exec. After every r prints it sends a request
 * to the parent over a System V message queue and waits for the answer.
 * The parent asks the user for a PID and sends it back on the queue:
 *
 *     -2   the child removes the queue and exits, then the parent exits
 *     -1   the child resumes printing
 *     pid  the child describes the process, kills it and resumes printing
 *
 * The queue is created before fork() so both processes can use it.
 * Ctrl+C removes the queue and stops both processes.
 *
 * Build: gcc -Wall -o resource_monitor resource_monitor.c
 */
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <unistd.h>

#define QUEUE_KEY   0x4F534D51  /* fixed key so the SIGINT handlers can find the queue */
#define MSG_REQUEST 1           /* child -> parent: "send me a PID" */
#define MSG_COMMAND 2           /* parent -> child: the PID, -1 or -2 */
#define MAX_LINE    256

struct message {
    long type;
    int  value;
};
#define MSG_SIZE (sizeof(struct message) - sizeof(long))

/*
 * awk program: print "score pid cpu mem comm" for every ps line. The score
 * comes first so that "sort -rn" orders by it; comm comes last because it
 * may contain spaces.
 */
#define AWK_SCORE \
    "{ cmd = $4; for (i = 5; i <= NF; i++) cmd = cmd \" \" $i;" \
    "  printf \"%.2f %d %.1f %.1f %s\\n\", 3*$2 + 2*$3, $1, $2, $3, cmd }"

/*
 * Run stages[0] | stages[1] | ... | stages[n-1] and return the read end of
 * a pipe attached to the last stage's stdout. Child pids go into pids[].
 */
static int run_pipeline(char *const *stages[], int n, pid_t pids[])
{
    int prev = -1;

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
            if (prev != -1) {
                dup2(prev, STDIN_FILENO);   /* read from the previous stage */
                close(prev);
            }
            dup2(fd[1], STDOUT_FILENO);     /* write to the next one */
            close(fd[0]);
            close(fd[1]);
            execvp(stages[i][0], stages[i]);
            perror(stages[i][0]);
            _exit(127);
        }
        if (prev != -1)
            close(prev);
        close(fd[1]);
        prev = fd[0];
    }
    return prev;
}

static void wait_pipeline(const pid_t pids[], int n)
{
    for (int i = 0; i < n; i++)
        waitpid(pids[i], NULL, 0);
}

static int is_pipeline_pid(int pid, const pid_t pids[], int n)
{
    for (int i = 0; i < n; i++)
        if (pids[i] == pid)
            return 1;
    return 0;
}

static void remove_queue(void)
{
    int queue = msgget(QUEUE_KEY, 0);

    if (queue != -1)
        msgctl(queue, IPC_RMID, NULL);
}

/* printf() is not safe inside a signal handler, write() is. */
static void say(const char *s)
{
    ssize_t written = write(STDOUT_FILENO, s, strlen(s));
    (void)written;
}

/* child: monitoring */

static void print_top(int k, int iteration, int r)
{
    char  count[16], line[MAX_LINE];
    char *ps[]   = { "ps", "axo", "pid=,%cpu=,%mem=,comm=", NULL };
    char *awk[]  = { "awk", AWK_SCORE, NULL };
    char *sort[] = { "sort", "-rn", NULL };
    char *head[] = { "head", "-n", count, NULL };
    char *const *stages[] = { ps, awk, sort, head };
    pid_t pids[4];
    int   shown = 0;
    FILE *in;

    /*
     * ps reports ~100% CPU for itself because it has only just started, so
     * ask for a few extra rows and skip the pipeline's own processes.
     */
    snprintf(count, sizeof count, "%d", k + 4);
    in = fdopen(run_pipeline(stages, 4, pids), "r");

    printf("\n[child] iteration %d/%d: top %d by usage_score = 3*CPU%% + 2*MEM%%\n",
           iteration, r, k);
    printf("%8s  %-20s %7s %7s %9s\n", "PID", "COMMAND", "CPU%", "MEM%", "SCORE");

    while (shown < k && fgets(line, sizeof line, in) != NULL) {
        double score, cpu, mem;
        int pid, at;

        if (sscanf(line, "%lf %d %lf %lf %n", &score, &pid, &cpu, &mem, &at) != 4)
            continue;
        if (is_pipeline_pid(pid, pids, 4))
            continue;
        line[strcspn(line, "\n")] = '\0';
        printf("%8d  %-20.20s %7.1f %7.1f %9.2f\n", pid, line + at, cpu, mem, score);
        shown++;
    }
    fflush(stdout);
    fclose(in);
    wait_pipeline(pids, 4);
}

/* Print a description of one process. Returns -1 if it does not exist. */
static int describe(int pid)
{
    char  pidstr[16], line[MAX_LINE], user[64];
    char *ps[] = { "ps", "p", pidstr, "o", "pid=,user=,%cpu=,%mem=,comm=", NULL };
    char *const *stages[] = { ps };
    pid_t pids[1];
    double cpu, mem;
    int   at, found = -1;
    FILE *in;

    snprintf(pidstr, sizeof pidstr, "%d", pid);
    in = fdopen(run_pipeline(stages, 1, pids), "r");

    if (fgets(line, sizeof line, in) != NULL &&
        sscanf(line, "%*d %63s %lf %lf %n", user, &cpu, &mem, &at) == 3) {
        line[strcspn(line, "\n")] = '\0';
        printf("[child] PID %d: command %s, owner %s, CPU %.1f%%, MEM %.1f%%, "
               "usage_score %.2f\n", pid, line + at, user, cpu, mem, 3 * cpu + 2 * mem);
        found = 0;
    }
    fclose(in);
    wait_pipeline(pids, 1);
    return found;
}

/* Ask the parent for a PID and act on the answer. */
static void handle_command(int queue)
{
    struct message msg = { MSG_REQUEST, 0 };
    int pid;

    printf("[child] waiting for a PID from the parent\n");
    fflush(stdout);

    msgsnd(queue, &msg, MSG_SIZE, 0);
    if (msgrcv(queue, &msg, MSG_SIZE, MSG_COMMAND, 0) == -1) {
        perror("[child] msgrcv");
        exit(EXIT_FAILURE);
    }
    pid = msg.value;

    if (pid == -2) {
        msgctl(queue, IPC_RMID, NULL);
        printf("[child] -2 received: message queue removed, exiting\n");
        exit(EXIT_SUCCESS);
    }
    if (pid == -1) {
        printf("[child] -1 received: no action, resuming\n");
        return;
    }
    if (pid <= 0 || describe(pid) == -1) {
        printf("[child] no process with PID %d, resuming\n", pid);
        return;
    }
    if (kill(pid, SIGKILL) == -1)
        printf("[child] could not kill PID %d: %s\n", pid, strerror(errno));
    else
        printf("[child] PID %d killed, resuming\n", pid);
}

static void child_sigint(int sig)
{
    (void)sig;
    remove_queue();
    _exit(EXIT_SUCCESS);
}

static void run_child(int queue, int n, int k, int r)
{
    signal(SIGINT, child_sigint);

    for (;;) {
        for (int i = 1; i <= r; i++) {
            print_top(k, i, r);
            sleep(n);
        }
        handle_command(queue);      /* exits on -2 */
    }
}

/* parent: user interaction */

/* Prompt until the user types an integer. Returns -1 at end of input. */
static int read_int(const char *prompt, int *value)
{
    char line[64];

    for (;;) {
        printf("%s", prompt);
        fflush(stdout);
        if (fgets(line, sizeof line, stdin) == NULL)
            return -1;
        if (sscanf(line, "%d", value) == 1)
            return 0;
        printf("please enter an integer\n");
    }
}

static void parent_sigint(int sig)
{
    (void)sig;
    say("\n[parent] Ctrl+C received, removing message queue\n");
    remove_queue();
    wait(NULL);
    _exit(EXIT_SUCCESS);
}

static void run_parent(int queue, pid_t child)
{
    struct message msg;

    signal(SIGINT, parent_sigint);

    for (;;) {
        if (msgrcv(queue, &msg, MSG_SIZE, MSG_REQUEST, 0) == -1)
            break;                  /* queue is gone: the child has exited */

        if (read_int("\n[parent] enter PID to kill (-1 = skip, -2 = quit): ", &msg.value) == -1)
            msg.value = -2;         /* end of input: shut down cleanly */

        msg.type = MSG_COMMAND;
        msgsnd(queue, &msg, MSG_SIZE, 0);
        if (msg.value == -2)
            break;
    }
    waitpid(child, NULL, 0);
    remove_queue();
    printf("[parent] monitor stopped\n");
}

int main(void)
{
    int n, k, r, queue;
    pid_t pid;

    if (read_int("seconds between prints (n): ", &n) == -1 || n <= 0 ||
        read_int("processes per print (k): ", &k) == -1 || k <= 0 ||
        read_int("prints before asking for a PID (r): ", &r) == -1 || r <= 0) {
        fprintf(stderr, "n, k and r must be positive integers\n");
        return EXIT_FAILURE;
    }

    remove_queue();                 /* drop a queue left behind by an earlier crash */
    queue = msgget(QUEUE_KEY, IPC_CREAT | 0600);
    if (queue == -1) {
        perror("msgget");
        return EXIT_FAILURE;
    }

    pid = fork();
    if (pid == -1) {
        perror("fork");
        msgctl(queue, IPC_RMID, NULL);
        return EXIT_FAILURE;
    }
    if (pid == 0)
        run_child(queue, n, k, r);  /* never returns */

    run_parent(queue, pid);
    return EXIT_SUCCESS;
}
