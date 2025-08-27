#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <limits.h>
#include <dirent.h>
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

static char *instances = NULL;
static char *OK = "ok\n";
static char *NEXIST = "nonexistent\n";

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
	char rootfs[PATH_MAX];
	snprintf(rootfs, PATH_MAX, "%s/rootfs/%s", ROOT, name);
	char rootfsmnt[PATH_MAX];
	snprintf(rootfsmnt, PATH_MAX, "%s/mnt", rootfs);

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
	char instances[PATH_MAX];
   	snprintf(instances, PATH_MAX, "%s/%s", ROOT, "instances");
	char rootfs[PATH_MAX];
	snprintf(rootfs, PATH_MAX, "%s/rootfs/%s", ROOT, name);
	char image[PATH_MAX];
	snprintf(image, PATH_MAX, "%s/images/%s", ROOT, "image.tar");

	if (file_contains(instances, name)) {
		write(out, NEXIST, strlen(NEXIST));
		return;
	}

	file_add(instances, name);
	mkdir(rootfs, 0755);
	clone_tar(image, rootfs);
	sync();

	write(out, OK, strlen(OK));
}

static void cmd_rm(int out, char *name) {
	char instances[PATH_MAX];
   	snprintf(instances, PATH_MAX, "%s/%s", ROOT, "instances");
	char rootfs[PATH_MAX];
	snprintf(rootfs, sizeof(rootfs), "%s/rootfs/%s", ROOT, name);

	if (!file_contains(instances, name)) {
		write(out, NEXIST, strlen(NEXIST));
		return;
	}

	clone_rm(rootfs);
	file_remove(instances, name);
	write(out, OK, strlen(OK));
}

static void cmd_run(int out, char *name) {
	char instances[PATH_MAX];
   	snprintf(instances, PATH_MAX, "%s/%s", ROOT, "instances");

	if (!file_contains(instances, name)) {
		write(out, NEXIST, strlen(NEXIST));
		return;
	}

	State *state = get_state();
	pthread_mutex_lock(&state->lock);
	// This instance is running
	if (strcmp(name, state->instance) == 0) {
    	set_frozen_cgroup(name, 0);
		pthread_mutex_unlock(&state->lock);
		write(out, OK, strlen(OK));
		stop_ctl();
		return;
	}
	// Another instance is running
	if (state->instance[0] != '\0') {
		kill_cgroup(state->instance);
		rm_cgroup(state->instance);
	}
	strcpy(state->instance, name);
	pthread_mutex_unlock(&state->lock);

	write(out, OK, strlen(OK));
	stop_ctl();

	int cgroup = new_cgroup(name);
	clone_init(cgroup, name);
	close(cgroup);
}

static void cmd_ls(int out, char *type) {
	if (strcmp(type, "image") == 0) {
		char images[PATH_MAX];
		snprintf(images, PATH_MAX, "%s/images", ROOT);

		DIR *dir = opendir(images);
    	if (!dir)
			die("images opendir");

    	struct dirent *de;
    	while ((de = readdir(dir)) != NULL) {
    	    if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
    	        continue;

    	    write(out, de->d_name, strlen(de->d_name));
    	    write(out, "\n", 1);
    	}

    	closedir(dir);
	}
	if (strcmp(type, "instance") == 0) {

	}
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
	if (strcmp(cmd, "ls") == 0)
		cmd_ls(out, arg);
}

void cmd(int in, int out) {
	char buf[256];
    int n;

	instances = malloc(PATH_MAX);

	while ((n = read(in, buf, sizeof(buf) - 1)) > 0)
        accept_cmd(out, buf, n);
}
