# Problem 3: logtop

## The task

`logtop <file> <column>` prints the five most frequent values in a
space-separated column of a log file, with their counts, exactly as

```sh
cut -d' ' -f<column> <file> | sort | uniq -c | sort -rn | head -5
```

The C code must not count or sort; it may only run those utilities and route
their output.

## How it works

The program opens the file, then starts five children connected by pipes:

```text
file -> cut -> sort -> uniq -c -> sort -rn -> head -5 -> pipe -> logtop -> stdout
```

`run_pipeline()` takes the list of argument vectors, the descriptor for the
first stage's stdin, and returns the read end of a pipe attached to the last
stage's stdout. Each child does the same three things: `dup2()` its stdin,
`dup2()` its stdout, `execvp()` the utility.

The parent then copies everything it reads from that pipe to its own stdout
and waits for all five children. Because the output is untouched, the format is
whatever `uniq -c` produced, which is the format shown in the problem.

## Details worth knowing

* The file is opened in C and passed to `cut` as stdin rather than as an
  argument. That way a missing file is reported by `logtop` itself with a clear
  message and a non-zero exit code.
* The column number is validated with `strtol`; anything that is not a positive
  integer is rejected.
* If a stage fails (for example `cut` with a bad delimiter), its exit status
  is non-zero and so is `logtop`'s.
* Fewer than five distinct values simply produce fewer lines, because `head -5`
  prints at most five.
