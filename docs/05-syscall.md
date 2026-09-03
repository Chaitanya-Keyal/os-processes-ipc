# Problem 5: the setnice_logged system call

## The task

Add `setnice_logged(int nice_val)` to Linux 6.14. It sets the caller's nice
value, rejects values outside -20..19 with `-EINVAL` without changing
anything, and on success logs the PID, command name, old and new nice value
to the kernel buffer.

## What nice is

Each process has a nice value from -20 (highest priority) to 19 (lowest). The
scheduler uses it to weight CPU time. Inside the kernel it is stored in
`task_struct` as `static_prio`; helpers convert between the two views:
`task_nice(p)` reads the nice value and `set_user_nice(p, nice)` sets it and
requeues the task correctly.

## The implementation

`kernel/setnice_logged.c`:

```c
SYSCALL_DEFINE1(setnice_logged, int, nice_val)
{
 int old_nice;

 if (nice_val < MIN_NICE || nice_val > MAX_NICE)
  return -EINVAL;
 old_nice = task_nice(current);
 set_user_nice(current, nice_val);

 pr_info("setnice_logged: pid=%d comm=%s old_nice=%d new_nice=%d\n",
  current->pid, current->comm, old_nice, task_nice(current));
 return 0;
}
```

* `SYSCALL_DEFINE1` is the kernel macro that declares a one-argument syscall
  with the right calling convention.
* `current` is the task that made the call.
* There is no privilege check, because the assignment asks only for the
  range check. The kernel's own `setpriority(2)` additionally calls
  `can_nice()` before lowering a nice value; that would be the one-line
  addition if the syscall were meant for real use.
* `pr_info()` writes to the kernel log, visible with `dmesg`.

## Wiring it into the kernel

Four edits, all in `setnice_logged.patch`:

1. New file `kernel/setnice_logged.c`.
2. `kernel/Makefile`: add `setnice_logged.o` to `obj-y` so it is always built
   into the kernel image.
3. `arch/x86/entry/syscalls/syscall_64.tbl`: add
   `470 common setnice_logged sys_setnice_logged`. The build turns this table
   into the array the kernel indexes on every syscall. 6.14's last number is
   466; the generator fills gaps with `sys_ni_syscall`, so 470 is fine and
   matches the number in the assignment's test program.
4. `include/linux/syscalls.h`: the prototype
   `asmlinkage long sys_setnice_logged(int nice_val);`.

## User-space side

There is no libc wrapper for a brand-new syscall, so the test program calls
`syscall(470, nice_val)` directly. It prints the nice value before and after
(via `getpriority`), then calls the syscall with 42 to show the `EINVAL` path.
A failed syscall returns -1 and sets `errno`, which is why `perror` works.

## Building

See `q5_syscall/README.md` for the exact commands. The short version: extract
6.14, `patch -p1`, copy the running kernel's config, `make localmodconfig` to
drop the thousands of modules the VM does not use, `make -j4`,
`make modules_install install`, reboot, then run `./test_setnice` and
`dmesg | grep setnice_logged`.
