#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <string.h>

#include <linux/vt.h>
#include <linux/sched.h>
#include <linux/types.h>

#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mount.h>

#define CGROUP_ROOT "/sys/fs/cgroup"

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

int main(int argc, char **argv) {
	if (argc != 2) {
		fprintf(stderr, "usage: %s <cgroup-subpath>\n", argv[0]);
		return 1;
	}

	int devnull = open("/dev/null", O_RDWR);
	dup2(devnull, STDIN_FILENO);
	dup2(devnull, STDOUT_FILENO);
	dup2(devnull, STDERR_FILENO);
	close(devnull);

	int fd = open("/dev/tty0", O_RDWR);
    	if (fd < 0)
    	    die("open /dev/tty0");

    	if (ioctl(fd, VT_ACTIVATE, 1) < 0)
    	    die("VT_ACTIVATE");

    	if (ioctl(fd, VT_WAITACTIVE, 1) < 0)
		die("VT_WAITACTIVATE");
	close(fd);	

	char path[4096];
    	snprintf(path, sizeof(path), "%s/%s", CGROUP_ROOT, argv[1]);

	if (mount("cgroup", CGROUP_ROOT, "cgroup2", 0, NULL) < 0 && errno != EBUSY)
		die("cgroup");
	
	if (mkdir(path, 0755) == -1 && errno != EEXIST)
		die("mkdir");

	int cfd = open(path, O_DIRECTORY);
	if (cfd < 0)
		die("cgroup open");

	pid_t pid = clone(cfd);
	if (pid < 0)
		die("clone3");
	if (pid > 0) return 0;
	close(cfd);

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

