# Problem 1: GCD over two pipes

## The task

The parent holds ten distinct positive integers. Each round it picks two at
random, removes them, prints them and sends them to a child. The child prints
the pair and its greatest common divisor g, sleeps `time(NULL) % g`
milliseconds, and sends g back. The parent prints g and sleeps g milliseconds.
The program ends after n/2 rounds or on Ctrl+C. No global variables.

## Design

Two pipes are created before `fork()` so both processes hold both:

```text
parent --(x, y)--> to_child   --> child
parent <---(g)---- to_parent  <-- child
```

After the fork each side closes the ends it does not use. The parent keeps the
write end of `to_child` and the read end of `to_parent`; the child keeps the
other two.

Picking without repetition is done with a small trick: choose a random index,
take that value, then move the array's last element into the hole and shrink
the length. Every remaining element is still in the array and the taken one is
gone, so nothing can be chosen twice.

The pair is sent as an `int[2]` in one `write()`. Small writes to a pipe are
atomic, so the child receives both numbers in one `read()`.

## Termination

* Normal: when fewer than two elements remain, the parent closes its write end
  and waits. The child's `read()` returns 0 (end of file) and its loop ends.
* Ctrl+C: the terminal delivers `SIGINT` to both processes. The parent's
  handler prints a message with `write()` and calls `_exit()`. The child either
  dies from the signal directly or sees end of file on the pipe. Nothing is
  shared between handler and program, so no global is needed.

## Things to notice in the code

* `fflush(stdout)` after every `printf`, because both processes write to the
  same terminal and stdio buffers output.
* `time(NULL) % g` is well defined because g is at least 1.
* Sleeping uses `usleep(ms * 1000)`.
