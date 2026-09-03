# CS F372 - Assignment 1
# Builds every user-space program. Problem 5's kernel part is documented in
# q5_syscall/README.md (it needs a kernel rebuild and cannot be built here).

CC      = gcc
CFLAGS  = -Wall -Wextra

PROGRAMS = q1_gcd_pipes/gcd_pipes \
           q2_resource_monitor/resource_monitor \
           q3_logtop/logtop \
           q4_belt_shell/belt_shell \
           q5_syscall/test_setnice

all: $(PROGRAMS)

%: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(PROGRAMS)

# Bundle sources, executables, readme and patch for submission.
dist: all
	tar czf assignment1_submission.tar.gz --exclude=.clang-format README.md Makefile \
	    q1_gcd_pipes q2_resource_monitor q3_logtop q4_belt_shell q5_syscall

.PHONY: all clean dist
