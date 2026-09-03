# CS F372 Operating Systems - Assignment 1

## Group details

| Name | ID number |
|------|-----------|
| Chaitanya Keyal | 2023B4A70727H |
| Arpit Wallecha  | 2023B3A70482H |
| Vishesh Agarwal | 2023B4A70611H |
| Abhishek Verma  | 2023B4A71256H |

## Contents

Longer explanations of each problem, the concepts behind them and a demo
script are in `docs/`.

| Directory              | Problem                                   | Program            |
|------------------------|-------------------------------------------|--------------------|
| `q1_gcd_pipes/`        | 1 - fork + two unnamed pipes, GCD exchange | `gcd_pipes`        |
| `q2_resource_monitor/` | 2 - weighted resource monitor (SysV MQ)    | `resource_monitor` |
| `q3_logtop/`           | 3 - log frequency analyzer                 | `logtop`           |
| `q4_belt_shell/`       | 4 - conveyor-belt shell                    | `belt_shell`       |
| `q5_syscall/`          | 5 - `setnice_logged` kernel system call    | `test_setnice` + kernel patch |

Tested on Ubuntu 24.04 with gcc 13 and procps-ng 4.0.4.

## Build

```bash
make            # builds all five user-space executables with -Wall -Wextra
make clean
make dist       # assignment1_submission.tar.gz with sources + executables
```

Each program is a single self-contained C file and can also be built by hand,
e.g. `gcc -Wall -o logtop q3_logtop/logtop.c`.

---

## Problem 1 - `gcd_pipes`

```bash
cd q1_gcd_pipes && ./gcd_pipes
```

* Two pipes are created before `fork()`: one carries x and y to the child, the
  other carries g back. Each process closes the ends it does not use.
* The parent removes two random elements per round by moving the last element
  into the hole, so nothing is picked twice. It prints x and y, writes them,
  reads g, prints it and sleeps g ms.
* The child reads x and y, prints x, y and gcd, sleeps `time(NULL) % g` ms and
  writes g back.
* After n/2 rounds the parent closes its write end; the child's `read()` returns
  0 and it exits. On Ctrl+C the parent prints a message and exits, which closes
  the pipes and ends the child the same way. No global variables.

## Problem 2 - `resource_monitor`

```bash
cd q2_resource_monitor && ./resource_monitor
seconds between prints (n): 2
processes per print (k): 5
prints before asking for a PID (r): 3
```

* `usage_score = 3*CPU% + 2*MEM%`. The child builds
  `ps axo pid=,%cpu=,%mem=,comm= | awk | sort -rn | head` with
  `pipe/fork/dup2/execvp`; awk computes the score and prints it first so that
  `sort -rn` orders by it. The child prints PID, command, CPU%, MEM% and score.
  The pipeline's own short-lived processes are skipped so they do not show up.
* A System V message queue (fixed key) is created before `fork()`. The child
  sends a request after r prints and blocks in `msgrcv()`; the parent, which was
  blocked waiting for that request, prompts for a PID and sends it back.
  * `-2`: the child removes the queue and exits, the parent reaps it and exits.
  * `-1`: the child resumes printing.
  * otherwise: the child prints the process's command, owner, CPU%, MEM% and
    score using `ps p <pid> o ...`, sends `SIGKILL`, confirms and resumes.
* Ctrl+C: both processes have a handler that finds the queue by its key, removes
  it and exits. No global variables.

## Problem 3 - `logtop`

```bash
cd q3_logtop && ./logtop access.log 1
      5 192.168.1.10
      4 192.168.1.14
      3 10.0.0.5
      2 10.0.0.9
      1 172.16.0.3
```

* No counting or sorting in C. Five children run
  `cut -d' ' -f<col> | sort | uniq -c | sort -rn | head -5`, connected with
  pipes; the log file is opened in C and becomes `cut`'s stdin.
* The last stage writes into a pipe and the program copies it to stdout, then
  waits for all stages. Fewer than 5 distinct values print fewer lines. A bad
  column or missing file gives an error and a non-zero exit status.

## Problem 4 - `belt_shell`

```bash
cd q4_belt_shell && ./belt_shell
belt-control$ add_item box1
belt-control$ list_items
box1
belt-control$ date
belt-control$ ping google.com
belt-control$ quit
```

* Loop: prompt, `fgets` (1023 characters max), `strtok` into words, run.
* `add_item`, `list_items` and `quit` run inside the shell; the queue is a local
  array of 10 items with errors for "full" and "missing name".
* `date` and `ping -c 4 <address>` are forked; the child `execvp`s and the
  parent `waitpid`s. Unknown commands print an error.
* Ctrl+C: the handler only sets a flag. The main loop prints
  `[ALERT] Emergency stop triggered, item queue cleared`, empties the queue and
  continues. A running `ping` is stopped by the Ctrl+C but the shell survives.
  Only `quit` exits.

## Problem 5 - `setnice_logged`

See `q5_syscall/README.md` for the full walkthrough. In short: apply
`q5_syscall/setnice_logged.patch` to the Linux 6.14 source (new file
`kernel/setnice_logged.c`, `obj-y` entry, syscall number 470 in
`syscall_64.tbl`, prototype in `syscalls.h`), rebuild and boot the kernel, then

```bash
cd q5_syscall && gcc -Wall -o test_setnice test_setnice.c && ./test_setnice
sudo dmesg | grep setnice_logged
```
