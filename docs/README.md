# Documentation

These notes explain the five problems in plain language: what is being asked,
which operating-system ideas are involved, how the code solves it and what to
look at during the demo. Read `concepts.md` first if fork, pipes, signals or
message queues are new to you.

| File                     | What it covers                                        |
|--------------------------|-------------------------------------------------------|
| `concepts.md`            | The building blocks used everywhere: fork, exec, wait, pipes, dup2, signals, System V message queues, system calls |
| `01-gcd-pipes.md`        | Problem 1: parent and child exchanging numbers over two pipes |
| `02-resource-monitor.md` | Problem 2: a top-like monitor with a message queue between parent and child |
| `03-logtop.md`           | Problem 3: chaining Unix utilities from C                |
| `04-belt-shell.md`       | Problem 4: a tiny shell with internal and external commands and Ctrl+C handling |
| `05-syscall.md`          | Problem 5: adding a system call to Linux 6.14            |
| `demo.md`                | Step-by-step demo script and likely viva questions       |
