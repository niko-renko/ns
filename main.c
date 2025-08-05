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

pid_t clone(int cfd) {
	struct clone_args args;
	memset(&args, 0, sizeof(args));
	args.flags = CLONE_INTO_CGROUP | CLONE_NEWPID | CLONE_NEWNS | CLONE_NEWCGROUP;
	args.exit_signal = SIGCHLD;
	args.cgroup = cfd;
	return syscall(SYS_clone3, &args, sizeof(args));
}

void die(const char *msg) {
	perror(msg);
	exit(1);
}

int main() {
	if (mount("cgroup", "/sys/fs/cgroup", "cgroup2", 0, NULL) < 0)
		die("cgroup");
	
	if (mkdir("/sys/fs/cgroup/test", 0755) == -1 && errno != EEXIST)
		die("mkdir");

	int cfd = open("/sys/fs/cgroup/test", O_DIRECTORY);
	if (cfd < 0)
		die("cgroup open");

	pid_t pid = clone(cfd);
	if (pid < 0)
		die("clone3");
	if (pid > 0) return 0;

	if (mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL) < 0)
	    die("mount MS_PRIVATE failed");

	if (mount("/root/Code/rootfs", "/root/Code/rootfs", NULL, MS_BIND | MS_REC, NULL) < 0)
		die("bind /mnt/newroot");
	
	if (syscall(SYS_pivot_root, "/root/Code/rootfs", "/root/Code/rootfs/mnt") < 0)
		die("pivot_root");

	if (umount2("/mnt", MNT_DETACH) < 0)
		die("umount2");

	execl("/sbin/init", "init", (char *)NULL);
	// Unreachable
}

