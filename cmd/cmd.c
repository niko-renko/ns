#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>

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
#include "../cgroup/cgroup.h"
#include "../vt/vt.h"

#define ROOT "/root/Code"

static char *instances = NULL;

static void clone_tar(const char *tar, const char *dest) {
	pid_t pid = fork();
    if (pid < 0)
        die("fork");
	if (pid > 0)
		if (waitpid(pid, NULL, 0) == -1)
			die("waitpid");
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
	clean_fds();
	execl("/bin/rm", "-rf", path, (char *) NULL);
}

static pid_t clone(int cfd) {
	struct clone_args args;
	memset(&args, 0, sizeof(args));
	args.flags = CLONE_INTO_CGROUP | CLONE_NEWPID | CLONE_NEWNS | CLONE_NEWCGROUP;
	args.exit_signal = SIGCHLD;
	args.cgroup = cfd;
	return syscall(SYS_clone3, &args, sizeof(args));
}

static void cmd_new(int cfd, char *name) {
	if (file_contains(instances, name)) {
		write(cfd, "exists\n", 7);
		return;
	}

	char rootfs[256];
	snprintf(rootfs, sizeof(rootfs), "%s/%s", ROOT, name);
	char image[256];
	snprintf(image, sizeof(image), "%s/%s", ROOT, "image.tar");

	file_add(instances, name);
	mkdir(rootfs, 0755);
	clone_tar(image, rootfs);
	write(cfd, "ok\n", 3);
}

static void cmd_rm(int cfd, char *name) {
	if (!file_contains(instances, name)) {
		write(cfd, "notfound\n", 9);
		return;
	}

	char rootfs[256];
	snprintf(rootfs, sizeof(rootfs), "%s/%s", ROOT, name);

	clone_rm(rootfs);
	file_remove(instances, name);
	write(cfd, "ok\n", 3);
}

static void cmd_run(int cfd, char *name) {
	State *state = get_state();
	pthread_mutex_lock(&state->lock);
	state->ctl = 0;
	pthread_mutex_unlock(&state->lock);

	switch_vt(1);

	int cgroup = new_cgroup(name);
	pid_t pid = clone(cgroup);
	if (pid < 0)
		die("clone3");
	if (pid > 0) {
		close(cgroup);
		write(cfd, "ok\n", 3);
		return;
	}
	clean_fds();

	if (mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL) < 0)
	    die("mount MS_PRIVATE failed");

	char rootfs[256];
	snprintf(rootfs, sizeof(rootfs), "%s/%s", ROOT, name);

	char rootfsmnt[256];
	snprintf(rootfsmnt, sizeof(rootfsmnt), "%s/%s/mnt", ROOT, name);

	if (mount(rootfs, rootfs, NULL, MS_BIND | MS_REC, NULL) < 0)
		die("bind /mnt/newroot");
	
	if (syscall(SYS_pivot_root, rootfs, rootfsmnt) < 0)
		die("pivot_root");

	if (umount2("/mnt", MNT_DETACH) < 0)
		die("umount2");

	execl("/sbin/init", "init", (char *)NULL);
}

static void accept_cmd(int cfd, char *line, int n) {
	if (instances == NULL) {
		instances = malloc(256);
    	snprintf(instances, 256, "%s/%s", ROOT, "instances");
	}

	line[n] = '\0';
	char *nl = strchr(line, '\n');
	if (nl) *nl = '\0';
	char *cmd = strtok(line, " ");
	char *arg = strtok(NULL, " ");

	if (!cmd)
		return;

	if (strcmp(cmd, "new") == 0)
		cmd_new(cfd, arg);
	if (strcmp(cmd, "rm") == 0)
		cmd_rm(cfd, arg);
	if (strcmp(cmd, "run") == 0)
		cmd_run(cfd, arg);
}

void cmd(int in, int out) {
	char buf[256];
    int n;

	while ((n = read(in, buf, sizeof(buf) - 1)) > 0)
        accept_cmd(out, buf, n);
}