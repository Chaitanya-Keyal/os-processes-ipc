/*
 * CS F372 Assignment 1, Problem 1
 *
 * The parent picks two random, distinct elements x and y from its array,
 * prints them and sends them to the child through a pipe. The child computes
 * g = gcd(x, y), prints x, y and g, sleeps (time(NULL) % g) ms and sends g
 * back through a second pipe. The parent prints g and sleeps g ms.
 * The loop ends when the array is exhausted (n/2 rounds) or on Ctrl+C.
 *
 * Build: gcc -Wall -o gcd_pipes gcd_pipes.c
 */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static int gcd(int a, int b)
{
    while (b != 0) {
        int t = a % b;
        a = b;
        b = t;
    }
    return a;
}

static void sleep_ms(long ms)
{
    usleep(ms * 1000);
}

/* Remove and return a random element of arr[0..*n). */
static int take_random(int arr[], int *n)
{
    int i = rand() % *n;
    int value = arr[i];

    arr[i] = arr[*n - 1];       /* fill the hole with the last element */
    (*n)--;
    return value;
}

/* printf() is not safe inside a signal handler, write() is. */
static void say(const char *s)
{
    ssize_t written = write(STDOUT_FILENO, s, strlen(s));
    (void)written;
}

/*
 * Ctrl+C: the parent reports and exits. That closes its pipe ends, so the
 * child's next read() returns 0 and the child exits as well.
 */
static void on_sigint(int sig)
{
    (void)sig;
    say("\n[parent] Ctrl+C received, stopping\n");
    _exit(EXIT_SUCCESS);
}

static void child(int in, int out)
{
    int pair[2];

    /* read() returns 0 once the parent closes its end: all rounds are done */
    while (read(in, pair, sizeof pair) == sizeof pair) {
        int x = pair[0], y = pair[1];
        int g = gcd(x, y);
        long ms = time(NULL) % g;

        printf("[child]  x = %d, y = %d, gcd = %d, sleeping %ld ms\n", x, y, g, ms);
        fflush(stdout);
        sleep_ms(ms);

        if (write(out, &g, sizeof g) != sizeof g)
            break;
    }
}

static void parent(int arr[], int n, int out, int in)
{
    int round = 0;

    while (n >= 2) {
        int pair[2], g;

        pair[0] = take_random(arr, &n);
        pair[1] = take_random(arr, &n);
        round++;

        printf("[parent] round %d: x = %d, y = %d\n", round, pair[0], pair[1]);
        fflush(stdout);

        if (write(out, pair, sizeof pair) != sizeof pair)
            break;
        if (read(in, &g, sizeof g) != sizeof g)
            break;

        printf("[parent] received g = %d, sleeping %d ms\n", g, g);
        fflush(stdout);
        sleep_ms(g);
    }
    printf("[parent] array exhausted after %d rounds\n", round);
}

int main(void)
{
    int arr[] = { 18, 24, 35, 49, 10, 63, 27, 40, 14, 21 };
    int n = sizeof arr / sizeof arr[0];
    int to_child[2], to_parent[2];
    pid_t pid;

    if (n % 2 != 0) {
        fprintf(stderr, "n must be even\n");
        return EXIT_FAILURE;
    }
    if (pipe(to_child) == -1 || pipe(to_parent) == -1) {
        perror("pipe");
        return EXIT_FAILURE;
    }

    pid = fork();
    if (pid == -1) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        close(to_child[1]);
        close(to_parent[0]);
        child(to_child[0], to_parent[1]);
        return EXIT_SUCCESS;
    }

    signal(SIGINT, on_sigint);
    srand(time(NULL));
    close(to_child[0]);
    close(to_parent[1]);
    parent(arr, n, to_child[1], to_parent[0]);

    close(to_child[1]);         /* child sees end of file and exits */
    wait(NULL);
    return EXIT_SUCCESS;
}
