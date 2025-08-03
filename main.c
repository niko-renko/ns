#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/mount.h>

pid_t clone() {
	struct clone_args args;
	memset(&args, 0, sizeof(args));
	args.flags = CLONE_NEWPID | CLONE_NEWNS;
	args.exit_signal = SIGCHLD;
	return syscall(SYS_clone3, &args, sizeof(args));
}

int main() {
	pid_t pid = clone();
	if (pid < 0) {
		perror("clone3");
		return 1;
	}
	if (pid > 0) return 0;
	
	int fd = open("/dev/tty9", O_RDWR);
	if (fd < 0) {
	        perror("open");
	        return 1;
	}
	
	for (int i = 0; i <= 2; i++) {
	    if (dup2(fd, i) < 0) {
	    	perror("dup2");
	    	return 1;
	    }
	}
	if (fd > 2) close(fd);
	
	if (setsid() < 0) {
	        perror("setsid");
	        return 1;
	}

	if (mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL) < 0) {
	    perror("mount MS_PRIVATE failed");
	    return 1;
	}

	umount("/proc");
	mount("proc", "/proc", "proc", 0, NULL);
	
	execl("/bin/bash", "bash", (char *)NULL);
	// Unreachable
}

