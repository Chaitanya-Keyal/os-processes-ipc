# Problem 4: belt_shell

## The task

A tiny shell with the prompt `belt-control$`. Internal commands are handled
in the shell process: `add_item <name>` (queue of at most 10), `list_items`,
`quit`. External commands are run with fork/exec/wait: `date` and
`ping <address>` (exactly four pings). Unknown commands print an error. Ctrl+C
must not exit the shell; it prints an alert, clears the queue and continues.

## The loop

```
print prompt -> fgets a line -> strtok into words -> run_command
```

`split()` turns the line into an argv array in place using `strtok` with
spaces, tabs and newline as separators. `run_command()` compares `argv[0]`
with the known commands.

Internal commands are ordinary functions on a `struct belt`, which holds a
fixed array of ten strings and a count. `add_item` reports a missing name or a
full queue; `list_items` prints the items in insertion order or
`Queue is empty`.

External commands go through `run_external()`: fork, in the child reset
`SIGINT` to its default and `execvp`, in the parent `waitpid`. `ping` is run as
`ping -c 4 <address>`.

## Ctrl+C

The handler is installed with `sigaction()` without `SA_RESTART`. It does one
thing: set a `volatile sig_atomic_t` flag. That is the only safe thing to do in
a handler, and it is the one global in this program (Problem 4 does not forbid
globals).

The main loop then reacts:

* At the prompt: `fgets()` returns NULL with `errno == EINTR`. The loop clears
  the error state and goes back to the top, where the flag is seen. It prints
  `[ALERT] Emergency stop triggered, item queue cleared`, sets the count to 0
  and prints a new prompt.
* While `ping` runs: the child has the default action and dies from the
  signal; `waitpid()` returns, the loop comes back to the top and handles the
  flag the same way.

End of input (Ctrl+D) ends the shell as well, since otherwise it would loop
forever on an empty stdin.
