# Problem 5 - `setnice_logged` system call (Linux 6.14)

`setnice_logged(int nice_val)` sets the calling process's nice value to
`nice_val` (-20 .. 19), returns `-EINVAL` for anything outside that range
without changing anything, and on success logs the PID, command name, old
nice and new nice value to the kernel ring buffer.

## Files

| File                   | Purpose                                                              |
|------------------------|----------------------------------------------------------------------|
| `setnice_logged.c`     | The system call. Goes to `kernel/setnice_logged.c` in the source tree |
| `setnice_logged.patch` | Everything needed, as one patch against `v6.14` (4 files)             |
| `test_setnice.c`       | User-space test program (wrapper around `syscall(470, ...)`)          |

The patch touches:

1. `kernel/setnice_logged.c` - new file with `SYSCALL_DEFINE1(setnice_logged, int, nice_val)`.
2. `kernel/Makefile` - adds `setnice_logged.o` to `obj-y` so it is always built in.
3. `arch/x86/entry/syscalls/syscall_64.tbl` - assigns number **470**:
   `470  common  setnice_logged  sys_setnice_logged`
   (6.14's last syscall is 466; the table generator pads the gap 467-469 with
   `sys_ni_syscall`, so 470 is legal and matches the number in the assignment's test).
4. `include/linux/syscalls.h` - prototype `asmlinkage long sys_setnice_logged(int nice_val);`

## Implementation notes

* Range check uses the kernel's own `MIN_NICE`/`MAX_NICE` from `linux/sched/prio.h`.
* No privilege check is made, as the assignment only asks for the range check.
  (The kernel's own `setpriority(2)` additionally requires `CAP_SYS_NICE` to
  lower the nice value; adding `can_nice()` would give that behaviour.)
* `set_user_nice(current, nice_val)` is the scheduler's exported helper; it updates
  `static_prio`/`prio` and requeues the task correctly. `task_nice()` reads the
  current value back.
* `pr_info()` writes to the kernel log buffer (`dmesg`).

## Build & install (Ubuntu 24.04 VM, 4 cores, 50 GB disk)

```bash
sudo apt update
sudo apt install -y build-essential libncurses-dev bison flex libssl-dev \
                    libelf-dev bc dwarves zstd fakeroot

cd ~
wget https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-6.14.tar.xz
tar xf linux-6.14.tar.xz
cd linux-6.14

# apply the patch (or copy setnice_logged.c and edit the 3 files by hand)
patch -p1 < /path/to/q5_syscall/setnice_logged.patch

# start from the running kernel's config, keep only the modules this VM uses
cp /boot/config-$(uname -r) .config
yes "" | make localmodconfig    # ~6000 modules -> ~60, cuts the build to ~20 min
scripts/config --disable SYSTEM_TRUSTED_KEYS --disable SYSTEM_REVOCATION_KEYS \
               --disable DEBUG_INFO --enable DEBUG_INFO_NONE --disable DEBUG_INFO_BTF \
               --set-str CONFIG_LOCALVERSION "-setnice"
make olddefconfig

make -j$(nproc)                 # ~20 min on 4 cores with the trimmed config
sudo make modules_install
sudo make install               # installs vmlinuz-6.14.0, updates GRUB
sudo reboot
```

After the reboot, select the 6.14.0 kernel in GRUB if it is not the default and check:

```bash
uname -r                         # 6.14.0-setnice
```

## Test

```bash
cd /path/to/q5_syscall
gcc -Wall -o test_setnice test_setnice.c
./test_setnice                   # sets nice 5, then tries 42 (must fail with EINVAL)
sudo dmesg | grep setnice_logged
```

Expected output:

```
pid 4321: nice before = 0
Nice value successfully changed.
pid 4321: nice after  = 5
setnice_logged(42) correctly rejected: Invalid argument
pid 4321: nice still  = 5
```

and in `dmesg`:

```
[  123.456789] setnice_logged: pid=4321 comm=test_setnice old_nice=0 new_nice=5
```

`./test_setnice -5` sets a negative nice value as well; no privilege is required.
