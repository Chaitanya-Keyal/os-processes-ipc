/*
 * CS F372 Assignment 1, Problem 4: belt_shell
 *
 * A small shell for a conveyor-belt controller. Loop: print the prompt, read
 * a line, split it with strtok, run the command.
 *
 * Internal commands run in the shell process itself:
 *     add_item <name>   append <name> to the belt queue (at most 10 items)
 *     list_items        print the queue in order, or "Queue is empty"
 *     quit              exit the shell
 *
 * External commands are forked; the child exec()s and the parent waits:
 *     date              current date and time
 *     ping <address>    ping <address> exactly 4 times
 *
 * Ctrl+C never exits the shell. It prints
 *     [ALERT] Emergency stop triggered, item queue cleared
 * empties the queue and continues. Only quit exits.
 *
 * Build: gcc -Wall -o belt_shell belt_shell.c
 */
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define PROMPT "belt-control$ "
#define MAX_ITEMS 10
#define MAX_LINE 1024 /* 1023 characters plus the terminating NUL */
#define MAX_ARGS 64

struct belt {
    char items[MAX_ITEMS][MAX_LINE];
    int count;
};

/*
 * The only thing shared with the signal handler. A handler cannot safely
 * touch the queue or call printf, so it just raises this flag and the main
 * loop performs the emergency stop.
 */
static volatile sig_atomic_t emergency_stop = 0;

static void on_sigint(int sig) {
    (void)sig;
    emergency_stop = 1;
}

static void add_item(struct belt* belt, const char* name) {
    if (name == NULL) {
        fprintf(stderr, "Error: add_item needs an item name\n");
        return;
    }
    if (belt->count == MAX_ITEMS) {
        fprintf(stderr, "Error: item queue is full (%d items)\n", MAX_ITEMS);
        return;
    }
    strcpy(belt->items[belt->count], name);
    belt->count++;
}

static void list_items(const struct belt* belt) {
    if (belt->count == 0) printf("Queue is empty\n");
    for (int i = 0; i < belt->count; i++) printf("%s\n", belt->items[i]);
}

/* Fork, exec the command in the child, wait for it in the parent. */
static void run_external(char* const argv[]) {
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        return;
    }
    if (pid == 0) {
        signal(SIGINT,
               SIG_DFL); /* Ctrl+C should stop the program, not the shell */
        execvp(argv[0], argv);
        fprintf(stderr, "Error: cannot run %s: %s\n", argv[0], strerror(errno));
        _exit(127);
    }
    while (waitpid(pid, NULL, 0) == -1 &&
           errno == EINTR); /* interrupted by Ctrl+C: keep waiting */
}

/* Split line in place into argv[]. Returns the number of words. */
static int split(char* line, char* argv[]) {
    int argc = 0;
    char* word = strtok(line, " \t\n");

    while (word != NULL && argc < MAX_ARGS - 1) {
        argv[argc++] = word;
        word = strtok(NULL, " \t\n");
    }
    argv[argc] = NULL;
    return argc;
}

/* Returns 1 when the shell should exit. */
static int run_command(struct belt* belt, int argc, char* argv[]) {
    if (argc == 0) return 0;

    if (strcmp(argv[0], "quit") == 0) return 1;

    if (strcmp(argv[0], "add_item") == 0) {
        add_item(belt, argc > 1 ? argv[1] : NULL);
    } else if (strcmp(argv[0], "list_items") == 0) {
        list_items(belt);
    } else if (strcmp(argv[0], "date") == 0) {
        char* date[] = {"date", NULL};
        run_external(date);
    } else if (strcmp(argv[0], "ping") == 0) {
        if (argc < 2 || argv[1][0] == '-') {
            fprintf(stderr, "Error: ping needs an address\n");
        } else {
            char* ping[] = {"ping", "-c", "4", argv[1], NULL};
            run_external(ping);
        }
    } else {
        fprintf(stderr, "Error: unknown command '%s'\n", argv[0]);
    }
    return 0;
}

int main(void) {
    struct belt belt = {.count = 0};
    struct sigaction sa = {0};
    char line[MAX_LINE];
    char* argv[MAX_ARGS];

    /* no SA_RESTART: a blocked fgets() returns with EINTR on Ctrl+C */
    sa.sa_handler = on_sigint;
    sigaction(SIGINT, &sa, NULL);

    for (;;) {
        if (emergency_stop) {
            emergency_stop = 0;
            printf("\n[ALERT] Emergency stop triggered, item queue cleared\n");
            belt.count = 0;
        }

        printf("%s", PROMPT);
        fflush(stdout);

        if (fgets(line, sizeof line, stdin) == NULL) {
            if (errno == EINTR) { /* Ctrl+C at the prompt */
                clearerr(stdin);
                continue;
            }
            printf("\n"); /* end of input */
            break;
        }
        if (run_command(&belt, split(line, argv), argv)) break;
    }
    return EXIT_SUCCESS;
}
