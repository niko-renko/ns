#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include <linux/input.h>
#include <linux/vt.h>
#include <linux/kd.h>
#include <linux/sched.h>

#include <sys/mount.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>

void die(const char *msg) {
	perror(msg);
	//exit(1);
}

void clean_fds() {
	for (int fd = 0; fd < sysconf(_SC_OPEN_MAX); fd++)
        close(fd);
}
