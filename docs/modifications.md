# Making changes on the spot

The viva asks each member to add or remove a feature. Every program is a
single C file; after editing, run `make` in `~/assignment-1` (or
`gcc -Wall -o name name.c` in the problem's folder). Below are the places to
touch for the kinds of changes that are likely to be asked.

## Problem 1: gcd_pipes.c

| Change                               | Where                                                        |
|--------------------------------------|--------------------------------------------------------------|
| Different numbers or array size      | `arr[]` in `main`. `n` is computed from it; keep it even     |
| Send LCM instead of GCD              | In `child`: `g = x / gcd(x, y) * y`                          |
| Child also sends x+y back            | Make the reply an `int[2]`, write it in `child`, read it in `parent` |
| Parent sleeps in seconds             | `sleep_ms(g)` -> `sleep(g)` in `parent`                      |
| Stop after a fixed number of rounds  | Add `&& round < LIMIT` to the `while` in `parent`            |
| Pick elements in order, not random   | In `take_random`, use `i = 0` instead of `rand() % *n`       |
| Print the remaining array each round | Loop over `arr[0..n)` after the two `take_random` calls      |

## Problem 2: resource_monitor.c

| Change                               | Where                                                        |
|--------------------------------------|--------------------------------------------------------------|
| Different weights in the score       | `3*$2 + 2*$3` inside `AWK_SCORE`, and the `3 * cpu + 2 * mem` in `describe` |
| Rank by CPU% only                    | Replace the score expression with `$2`                       |
| Ascending order (lowest first)       | `sort -rn` -> `sort -n` in `print_top`                        |
| Show the user of each process        | Add `user=` to the `ps` columns and a field to the awk printf and `sscanf` |
| Send SIGTERM instead of SIGKILL      | `kill(pid, SIGKILL)` in `handle_command`                     |
| Ask for a PID after every print      | Call `handle_command` inside the `for` loop in `run_child`   |
| Use a new code, e.g. -3 = print once more | Add another `if (pid == -3)` branch in `handle_command` |
| Different prompt text or table format | The `printf` calls in `run_parent` and `print_top`           |
| Child should also confirm to the parent | Send a third message type (`#define MSG_DONE 3`) from the child after acting and `msgrcv` it in `run_parent` |

Message layout: `struct message { long type; int value; }`. Add fields to it
if more than one integer must travel; `MSG_SIZE` adjusts automatically.

## Problem 3: logtop.c

| Change                               | Where                                                        |
|--------------------------------------|--------------------------------------------------------------|
| Top N instead of top 5               | `head[]` argv: `"-5"` -> `"-N"`, or read N from `argv[3]`    |
| Different delimiter (comma, tab)     | `cut[]` argv: the `" "` after `-d`                            |
| Least frequent values                | `sort_rn[]`: `"-rn"` -> `"-n"`                                |
| Case-insensitive counting            | Insert a stage `{ "tr", "A-Z", "a-z", NULL }` before `sort` and bump the stage count |
| Only count lines containing a word   | Insert `{ "grep", word, NULL }` as the first stage            |
| Print a header line                  | `printf` before `copy_to_stdout`                             |

Adding a stage: declare its argv array, put it in `stages[]` in the right
position, and update the `5` in `pids[5]`, the `run_pipeline` call and the
wait loop.

## Problem 4: belt_shell.c

| Change                               | Where                                                        |
|--------------------------------------|--------------------------------------------------------------|
| New internal command, e.g. `remove_item` | Write a function like `add_item`, add an `else if` in `run_command` |
| `count_items`                        | `printf("%d\n", belt->count)` behind a new `else if`         |
| `clear_items`                        | `belt->count = 0` behind a new `else if`                     |
| Queue size other than 10             | `MAX_ITEMS`                                                  |
| New external command, e.g. `ls`      | Copy the `date` branch with `{ "ls", NULL }`; extra arguments can be passed through by building the argv from `argv[1..]` |
| Ping a different number of times     | The `"4"` in the `ping` argv                                 |
| Different prompt                     | `PROMPT`                                                     |
| Ctrl+C should exit after printing    | Replace `belt.count = 0` with `break` in the flag check      |
| Reject duplicate items               | Loop over `belt->items` in `add_item` before storing         |

## Problem 5: setnice_logged.c (in ~/linux-6.14/kernel/)

| Change                               | Where                                                        |
|--------------------------------------|--------------------------------------------------------------|
| Also log the UID                     | Add `from_kuid(&init_user_ns, current_uid())` to the `pr_info` (include `<linux/uidgid.h>` and `<linux/cred.h>`) |
| Return the old nice value            | `return old_nice;` instead of `return 0` (user space then sees it as the syscall result) |
| Narrow the allowed range             | The two compares against `MIN_NICE`/`MAX_NICE`               |
| Refuse to lower nice without privilege | `if (nice_val < old_nice && !can_nice(current, nice_val)) return -EPERM;` |
| Change a different process by PID    | Add a `pid_t` argument (`SYSCALL_DEFINE2`), look it up with `find_task_by_vpid(pid)` under `rcu_read_lock()` |

Then rebuild and install:

```bash
cd ~/linux-6.14 && make -j4 && sudo make install && sudo reboot
```

Only the changed file is recompiled; the whole step takes a couple of
minutes. After the reboot, `./test_setnice` and `sudo dmesg | grep setnice_logged`
show the new behaviour. For the user-space side, the test's `main` in
`test_setnice.c` is where to add calls.
