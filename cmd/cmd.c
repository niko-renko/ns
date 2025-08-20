#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <limits.h>

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

#include "../common.h"
#include "../state/state.h"
#include "../set/set.h"
#include "../ctl/ctl.h"
#include "../cgroup/cgroup.h"

#define ROOT "/root/Code"

static char *instances = NULL;

static void clone_tar(const char *tar, const char *dest) {
	pid_t pid = fork();
    if (pid < 0)
        die("fork");
	if (pid > 0)
		if (waitpid(pid, NULL, 0) == -1)
			die("waitpid");
		else
			return;
	clean_fds();
	execl("/bin/tar", "tar", "xf", tar, "--strip-components=1", "-C", dest, (char *) NULL);
}

static void clone_rm(const char *path) {
	pid_t pid = fork();
    if (pid < 0)
        die("fork");
	if (pid > 0)
		if (waitpid(pid, NULL, 0) == -1)
			die("waitpid");
		else
			return;
	clean_fds();
	execl("/bin/rm", "rm", "-rf", path, (char *) NULL);
}

static void clone_init(int cgroup, const char *name) {
	struct clone_args args;
	memset(&args, 0, sizeof(args));
	args.flags = CLONE_INTO_CGROUP | CLONE_NEWPID | CLONE_NEWNS | CLONE_NEWCGROUP;
	args.exit_signal = SIGCHLD;
	args.cgroup = cgroup;
	pid_t pid = syscall(SYS_clone3, &args, sizeof(args));
    if (pid < 0)
        die("fork");
	if (pid > 0)
		return;
	clean_fds();

	char rootfs[256];
	char rootfsmnt[256];

	snprintf(rootfs, sizeof(rootfs), "%s/%s", ROOT, name);
	snprintf(rootfsmnt, sizeof(rootfsmnt), "%s/mnt", rootfs);

	if (mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL) < 0)
	    die("mount MS_PRIVATE failed");

	if (mount(rootfs, rootfs, NULL, MS_BIND | MS_REC, NULL) < 0)
		die("bind /mnt/newroot");
	
	if (syscall(SYS_pivot_root, rootfs, rootfsmnt) < 0)
		die("pivot_root");

	if (umount2("/mnt", MNT_DETACH) < 0)
		die("umount2");

	execl("/sbin/init", "init", (char *)NULL);
}

static void cmd_new(int out, char *name) {
	if (file_contains(instances, name)) {
		write(out, "exists\n", 7);
		return;
	}

	char rootfs[256];
	char image[256];
	snprintf(rootfs, sizeof(rootfs), "%s/%s", ROOT, name);
	snprintf(image, sizeof(image), "%s/%s", ROOT, "image.tar");

	file_add(instances, name);
	mkdir(rootfs, 0755);
	clone_tar(image, rootfs);
	write(out, "ok\n", 3);
}

static void cmd_rm(int out, char *name) {
	if (!file_contains(instances, name)) {
		write(out, "notfound\n", 9);
		return;
	}

	char rootfs[256];
	snprintf(rootfs, sizeof(rootfs), "%s/%s", ROOT, name);

	clone_rm(rootfs);
	file_remove(instances, name);
	write(out, "ok\n", 3);
}

static void cmd_run(int out, char *name) {
	if (!file_contains(instances, name)) {
		write(out, "notfound\n", 9);
		return;
	}

	write(out, "ok\n", 3);
	stop_ctl();

	//int instance = get_instance(state, name);
	//if (instance == state->active) {
	//	write(out, "active\n", 7);
	//	pthread_mutex_unlock(&state->lock);
	//	return;
	//}
	//if (instance == -1)
	//	instance = add_instance(state, name);
	// state->active = instance;

	//int cgroup = new_cgroup(name);
	//clone_init(cgroup, name);
}

static void accept_cmd(int out, char *line, int n) {
	line[n] = '\0';
	char *nl = strchr(line, '\n');
	if (nl) *nl = '\0';
	char *cmd = strtok(line, " ");
	char *arg = strtok(NULL, " ");

	if (!cmd)
		return;

	if (strcmp(cmd, "new") == 0)
		cmd_new(out, arg);
	if (strcmp(cmd, "rm") == 0)
		cmd_rm(out, arg);
	if (strcmp(cmd, "run") == 0)
		cmd_run(out, arg);
}

void cmd(int in, int out) {
	char buf[256];
    int n;

	instances = malloc(PATH_MAX);
   	snprintf(instances, PATH_MAX, "%s/%s", ROOT, "instances");

	while ((n = read(in, buf, sizeof(buf) - 1)) > 0)
        accept_cmd(out, buf, n);
}
