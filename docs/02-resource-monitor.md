# Problem 2: Weighted resource monitor

## The task

Read n, k and r. A child prints the k processes with the highest
`usage_score = 3*CPU% + 2*MEM%` every n seconds. After every r prints it asks
the parent for a PID over a System V message queue. -2 removes the queue and
ends the program, -1 skips, any other PID is described, killed and confirmed.
`ps` must be used with BSD syntax; no global variables.

## How the ranking is produced

The child does not parse `/proc` or compute rankings itself. It builds this
pipeline with `pipe()`, `fork()`, `dup2()` and `execvp()`:

```sh
ps axo pid=,%cpu=,%mem=,comm=  |  awk '...'  |  sort -rn  |  head -n k+4
```

* `ps axo ...` is BSD syntax (no leading dash). The `=` after each column
  suppresses the header.
* `awk` computes the score and prints `score pid cpu mem comm`. The score comes
  first so that `sort -rn` sorts by it numerically; the command name comes last
  because it may contain spaces.
* The child reads the result through a pipe, parses each line with `sscanf`
  and prints a formatted table.

One subtlety: `ps` reports about 100% CPU for itself because it has only just
started. To keep it and the other helpers out of the table, the child asks for
four extra rows and skips lines whose PID belongs to the pipeline it just
created.

`run_pipeline()` is the generic piece: for each stage it creates a pipe, forks,
and in the child redirects stdin to the previous pipe and stdout to the new
one before `execvp`. The parent closes what it no longer needs and remembers
the read end for the next stage.

## Parent and child coordination

The queue is created with `msgget()` before `fork()`, using a fixed key. Two
message types are used:

| Type | Direction       | Meaning                              |
|------|-----------------|--------------------------------------|
| 1    | child -> parent | "r prints done, send me a PID"        |
| 2    | parent -> child | the PID, -1 or -2                     |

The parent spends its life blocked in `msgrcv()` waiting for a type 1 message.
When one arrives it prompts the user, validates the input, and sends a type 2
message back. The child, which is blocked on type 2, then acts:

* `-2`: `msgctl(IPC_RMID)` removes the queue, the child exits. The parent, which
  knows it sent -2, waits for the child and exits too.
* `-1`: the child goes back to printing.
* PID: the child runs `ps p <pid> o pid=,user=,%cpu=,%mem=,comm=` to print the
  command, owner, CPU%, MEM% and score, then `kill(pid, SIGKILL)`, prints a
  confirmation and resumes. A PID that does not exist prints a message instead.

## Ctrl+C without globals

Both processes receive `SIGINT`. Each installs its own handler. The handlers
cannot read the queue id from a global, so they call `msgget(KEY, 0)` to look
the queue up again and remove it. The parent also waits for the child before
exiting so nothing is left behind. At start-up the program removes any queue
with the same key left over from an earlier crash.

## Trying it

```bash
sleep 600 &            # a victim, note its PID
./resource_monitor     # n=2, k=5, r=3
```

After three tables the prompt appears. Enter the sleep PID, then -1 at the next
prompt, then -2. `ipcs -q` afterwards shows no queue.
