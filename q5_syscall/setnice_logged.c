// SPDX-License-Identifier: GPL-2.0
/*
 * CS F372 - Assignment 1, Problem 5
 * setnice_logged(int nice_val) - custom system call for Linux 6.14
 *
 * Copy this file to  <linux-6.14>/kernel/setnice_logged.c  and add
 * "setnice_logged.o" to obj-y in kernel/Makefile (see setnice_logged.patch).
 *
 * Behaviour:
 *   - nice_val must be in [MIN_NICE, MAX_NICE] = [-20, 19]; otherwise
 *     -EINVAL is returned and nothing is changed.
 *   - On success the calling task's nice value is set to nice_val and a
 *     line is written to the kernel log (see: dmesg | grep setnice_logged).
 */
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/sched.h>
#include <linux/sched/prio.h>
#include <linux/syscalls.h>

SYSCALL_DEFINE1(setnice_logged, int, nice_val)
{
	int old_nice;

	/* Validate before touching anything. */
	if (nice_val < MIN_NICE || nice_val > MAX_NICE)
		return -EINVAL;

	old_nice = task_nice(current);
	set_user_nice(current, nice_val);

	pr_info("setnice_logged: pid=%d comm=%s old_nice=%d new_nice=%d\n",
		current->pid, current->comm, old_nice, task_nice(current));

	return 0;
}
