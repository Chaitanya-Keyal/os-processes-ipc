# Concepts used in this assignment

## Processes, fork, exec, wait

A process is a running program with its own memory. `fork()` creates a copy of
the calling process. Both copies continue from the same point; the only
difference is the return value: 0 in the child, the child's PID in the parent.

```c
pid_t pid = fork();
if (pid == 0) {
    /* child */
} else {
    /* parent; pid is the child's PID */
}
```

`exec()` (we use `execvp`) replaces the current process image with another
program. It never returns on success. The usual pattern for running a command
is therefore fork, then exec in the child, then wait in the parent:

```c
if (fork() == 0) {
    execvp("date", argv);   /* the child becomes "date" */
    _exit(127);             /* only reached if exec failed */
}
wait(NULL);                 /* parent waits for the child to finish */
```

`wait()` / `waitpid()` block until a child terminates and collect its exit
status. A child that has exited but not been waited for is a zombie, so every
child we create is waited for.

## Pipes and dup2

A pipe is a one-way channel inside the kernel. `pipe(fd)` gives two file
descriptors: `fd[0]` to read from, `fd[1]` to write to. Because file
descriptors survive `fork()`, a pipe created before forking is shared by parent
and child. Each side closes the end it does not use; that matters because a
reader only sees end-of-file when every write end is closed.

`dup2(old, new)` makes descriptor `new` refer to the same thing as `old`. With
`dup2(fd[1], STDOUT_FILENO)` a program's standard output goes into the pipe.
That is exactly how the shell implements `a | b`: fork `a` with stdout on the
pipe's write end and `b` with stdin on the read end. Problems 2 and 3 build such
chains by hand.

## Signals

A signal is an asynchronous notification from the kernel. Ctrl+C sends
`SIGINT` to every process in the terminal's foreground process group. The
default action is to terminate. A program can install a handler with
`signal()` or `sigaction()`.

Two rules for handlers matter here:

* Only async-signal-safe functions may be called inside one. `write()` is
  safe; `printf()` and `malloc()` are not.
* If a blocking call such as `read()` or `waitpid()` is interrupted and the
  handler was installed without `SA_RESTART`, the call returns -1 with
  `errno == EINTR`. Problem 4 relies on that to notice Ctrl+C at the prompt.

Problems 1 and 2 forbid global variables, so their handlers cannot rely on a
shared flag. Problem 1's handler just exits, which closes the pipes and ends
the child. Problem 2's handlers look the message queue up again by its fixed
key and remove it.

## System V message queues

A message queue lives in the kernel and outlives the process that created it
until someone removes it. Processes find it through a key.

```c
int q = msgget(KEY, IPC_CREAT | 0600);            /* create or open */
struct { long type; int value; } m = { 1, 42 };
msgsnd(q, &m, sizeof m.value, 0);                 /* send */
msgrcv(q, &m, sizeof m.value, 1, 0);              /* receive type 1, block */
msgctl(q, IPC_RMID, NULL);                        /* remove */
```

Every message starts with a `long` type, and `msgrcv` can wait for a specific
type. Problem 2 uses type 1 for "child asks for a PID" and type 2 for "parent
answers", so each side blocks only on the message it expects. The size passed
to `msgsnd`/`msgrcv` excludes the type field. `ipcs -q` lists queues that exist
on the system, `ipcrm -q <id>` removes one by hand.

## System calls

User programs cannot touch the kernel's data directly. They ask for services
through system calls: the program puts a number and arguments in registers and
executes a special instruction, the CPU switches to kernel mode, the kernel
looks the number up in its syscall table and runs the handler. `syscall(470, 5)`
from the C library does exactly that with number 470. Problem 5 adds a new
entry to that table.
