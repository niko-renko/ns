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

void die(const char *msg) {
	perror(msg);
	exit(1);
}

int main() {
	pid_t pid = clone();
	if (pid < 0)
		die("clone3");
	if (pid > 0) return 0;
	
	int fd = open("/dev/tty1", O_RDWR);
	if (fd < 0)
	        die("open");
	
	for (int i = 0; i <= 2; i++) {
	    if (dup2(fd, i) < 0)
	    	die("dup2");
	}
	if (fd > 2) close(fd);
	
	if (setsid() < 0)
	        die("setsid");

	if (mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL) < 0)
	    die("mount MS_PRIVATE failed");

	if (mount("/root/Code/custom/rootfs", "/root/Code/custom/rootfs", NULL, MS_BIND | MS_REC, NULL) < 0)
		die("bind /mnt/newroot");
	
	if (syscall(SYS_pivot_root, "/root/Code/custom/rootfs", "/root/Code/custom/rootfs/mnt") < 0)
		die("pivot_root");

	if (umount2("/mnt", MNT_DETACH) < 0)
		die("umount2");

	execl("/sbin/init", "init", (char *)NULL);
	// Unreachable
}

