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

#define ROOT "/root/Code"
#define CGROUP_ROOT "/sys/fs/cgroup"

static char *instances = NULL;

static int remove_recursive(const char *path) {
    char cmd[512];
    int ret;

    if (snprintf(cmd, sizeof(cmd), "rm -rf '%s'", path) >= (int)sizeof(cmd))
        return -1;

    ret = system(cmd);
    return (ret == 0) ? 0 : -1;
}

static pid_t clone(int cfd) {
	struct clone_args args;
	memset(&args, 0, sizeof(args));
	args.flags = CLONE_INTO_CGROUP | CLONE_NEWPID | CLONE_NEWNS | CLONE_NEWCGROUP;
	args.exit_signal = SIGCHLD;
	args.cgroup = cfd;
	return syscall(SYS_clone3, &args, sizeof(args));
}

static void spawn_tar(const char *tar, const char *dest) {
	pid_t pid = fork();
	if (pid == 0) {
		clean_fds();
		execl("/bin/tar", "tar", "xf", tar, "--strip-components=1", "-C", dest, (char *) NULL);
	}
	if (waitpid(pid, NULL, 0) == -1)
		die("waitpid");
}

static void cmd_new(int cfd, char *name) {
	if (file_contains(instances, name)) {
		write(cfd, "exists\n", 7);
		return;
	}

	file_add(instances, name);

	char rootfs[256];
	snprintf(rootfs, sizeof(rootfs), "%s/%s", ROOT, name);
	char image[256];
	snprintf(image, sizeof(image), "%s/%s", ROOT, "image.tar");

	mkdir(rootfs, 0755);
	spawn_tar(image, rootfs);

	write(cfd, "ok\n", 3);
}

static void cmd_rm(int cfd, char *name) {
	if (!file_contains(instances, name)) {
		write(cfd, "notfound\n", 9);
		return;
	}

	char rootfs[256];
	snprintf(rootfs, sizeof(rootfs), "%s/%s", ROOT, name);
	remove_recursive(rootfs);

	file_remove(instances, name);

	write(cfd, "ok\n", 3);
}

static void cmd_run(int cfd, char *name) {
	State *state = get_state();
	pthread_mutex_lock(&state->lock);
	state->ctl = 0;
	state->allow = 1;
	pthread_mutex_unlock(&state->lock);

	int tty0 = open("/dev/tty0", O_RDWR);
	if (tty0 < 0)
		die("tty0 open");

    if (ioctl(tty0, VT_ACTIVATE, 1) < 0)
    	die("VT_ACTIVATE");

    if (ioctl(tty0, VT_WAITACTIVE, 1) < 0)
		die("VT_WAITACTIVATE");
	close(tty0);

	char cgpath[4096];
    	snprintf(cgpath, sizeof(cgpath), "%s/%s", CGROUP_ROOT, name);

	if (mount("cgroup", CGROUP_ROOT, "cgroup2", 0, NULL) < 0 && errno != EBUSY)
		die("cgroup");
	
	if (mkdir(cgpath, 0755) == -1 && errno != EEXIST)
		die("mkdir");

	int fd = open(cgpath, O_DIRECTORY);
	if (fd < 0)
		die("cgroup open");

	pid_t pid = clone(fd);
	if (pid < 0)
		die("clone3");
	if (pid > 0) {
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

static void cmd_allow(int cfd, char *value) {
	int allow;
	if (value[0] == '0')
		allow = 0;
	else
		allow = 1;

	State *state = get_state();
	pthread_mutex_lock(&state->lock);
	state->allow = allow;
	pthread_mutex_unlock(&state->lock);
	write(cfd, "ok\n", 3);
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
	if (strcmp(cmd, "allow") == 0)
		cmd_allow(cfd, arg);
}

void cmd(int in, int out) {
	char buf[256];
    int n;

	while ((n = read(in, buf, sizeof(buf) - 1)) > 0)
        accept_cmd(out, buf, n);
}