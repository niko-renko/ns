#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <string.h>

#include <linux/sched.h>
#include <linux/types.h>

#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mount.h>

#include <dirent.h>

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
	
	int fd = open("/dev/tty1", O_RDWR);
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

	if (mount("/var/lib/lxc/test/rootfs", "/var/lib/lxc/test/rootfs", NULL, MS_BIND | MS_REC, NULL) < 0) {
		perror("bind /mnt/newroot");
		return 1;
	}
	
	if (syscall(SYS_pivot_root, "/var/lib/lxc/test/rootfs", "/var/lib/lxc/test/rootfs/mnt") < 0) {
		perror("pivot_root");
		return 1;
	}

	if (umount2("/mnt", MNT_DETACH) < 0) {
		perror("umount2");
		return 1;
	}

	// umount("/proc");
	// mount("proc", "/proc", "proc", 0, NULL);
	
	execl("/sbin/init", "init", (char *)NULL);
	// execl("/bin/bash", "bash", (char *)NULL);
	// execl("/oldroot/bin/bash", "bash", (char *)NULL);
	// Unreachable
}

