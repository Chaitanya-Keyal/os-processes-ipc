/*
 * CS F372 - Assignment 1, Problem 5
 * User-space test for the setnice_logged system call.
 *
 * Build: gcc -Wall -Wextra -o test_setnice test_setnice.c
 * Run:   ./test_setnice          (sets nice to 5, then tries an invalid value)
 *        ./test_setnice 10       (sets nice to 10)
 *        sudo dmesg | grep setnice_logged
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <unistd.h>

#define __NR_setnice_logged 470 /* number assigned in syscall_64.tbl */

/* Local wrapper around the raw system call. */
int setnice_logged(int nice_val)
{
	return syscall(__NR_setnice_logged, nice_val);
}

static int current_nice(void)
{
	errno = 0;
	return getpriority(PRIO_PROCESS, 0);
}

int main(int argc, char *argv[])
{
	int wanted = (argc > 1) ? atoi(argv[1]) : 5;
	int ret;

	printf("pid %d: nice before = %d\n", (int)getpid(), current_nice());

	ret = setnice_logged(wanted);
	if (ret == 0) {
		printf("Nice value successfully changed.\n");
	} else {
		fflush(stdout);
		perror("setnice_logged failed");
	}
	printf("pid %d: nice after  = %d\n", (int)getpid(), current_nice());

	/* Out-of-range value must be rejected with EINVAL and change nothing.
	 */
	ret = setnice_logged(42);
	if (ret == -1 && errno == EINVAL)
		printf("setnice_logged(42) correctly rejected: %s\n",
		       strerror(errno));
	else
		printf(
		    "UNEXPECTED: setnice_logged(42) returned %d (errno %d)\n",
		    ret, errno);
	printf("pid %d: nice still  = %d\n", (int)getpid(), current_nice());

	printf(
	    "Check the kernel log with:  sudo dmesg | grep setnice_logged\n");
	return EXIT_SUCCESS;
}
