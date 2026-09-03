# Demo script

Start the VirtualBox VM `cs-f372-ubuntu` and log in (`ssh osvm` from WSL, or
the VM console). Then:

```bash
cd ~/assignment-1 && make
```

## Problem 1

```bash
cd q1_gcd_pipes && ./gcd_pipes
```

Point out the parent's "x, y" lines, the child's gcd lines and the parent's
"received g" lines. Five rounds, then exit. Run it again and press Ctrl+C.

## Problem 2

In a second terminal: `sleep 600 &` and note the PID. Then:

```bash
cd ../q2_resource_monitor && ./resource_monitor      # n=2, k=5, r=3
```

After three tables it prompts. Enter the sleep PID: description, kill,
confirmation, then it resumes. Next prompt: -1. Next prompt: -2. Show
`ipcs -q` is empty.

## Problem 3

```bash
cd ../q3_logtop && ./logtop access.log 1
./logtop access.log 9
cut -d' ' -f1 access.log | sort | uniq -c | sort -rn | head -5
```

The last command shows the output matches the raw pipeline.

## Problem 4

```bash
cd ../q4_belt_shell && ./belt_shell
```

`add_item box1`, `add_item box2`, `list_items`, `date`, `ping google.com`,
`foo` (error), Ctrl+C (alert), `list_items` (empty), `quit`.

## Problem 5

```bash
uname -r                                  # 6.14.0-setnice
cd ../q5_syscall && gcc -Wall -o test_setnice test_setnice.c && ./test_setnice
sudo dmesg | grep setnice_logged
```

Source on request: `~/linux-6.14/kernel/setnice_logged.c` and
`grep setnice ~/linux-6.14/arch/x86/entry/syscalls/syscall_64.tbl`.

## If asked to change something

Edit the C file and run `make`. For the syscall: edit
`~/linux-6.14/kernel/setnice_logged.c`, then `make -j4 && sudo make install`
and reboot. Only the changed file is recompiled, so this takes a couple of
minutes.

## Likely viva questions

* Why close unused pipe ends? A reader only gets end of file when every write
  end is closed; leaving one open in the reader itself would hang it.
* Why `_exit` and `write` in a handler and not `exit`/`printf`? They are
  async-signal-safe; stdio is not and can deadlock if interrupted mid-call.
* Why create the message queue before `fork`? So both processes get the id.
  The queue is kernel-owned and must be removed explicitly or it stays after
  the program exits.
* Why does `ps` show 100% CPU for itself? Its CPU% is CPU time over elapsed
  time, and it has only just started.
* What does `dup2` do in the pipeline? Redirects a child's stdin/stdout to the
  pipe ends before `exec`, so the utility reads and writes the pipe without
  knowing it.
* Why is the syscall number 470 and not 467? Any unused number works; the
  table generator pads gaps. 470 matches the assignment's test stub.
* Could any user set -20 with this? Yes; the assignment asks only for the
  range check. The kernel's own `setpriority` adds a `can_nice()` privilege
  check, which would be the one-line fix.
