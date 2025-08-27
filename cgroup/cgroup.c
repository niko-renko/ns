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
#include "cgroup.h"

void init_cgroup(void) {
	char cgpath[PATH_MAX];

	if (mount("cgroup", CGROUP_ROOT, "cgroup2", 0, NULL) < 0 && errno != EBUSY)
		die("cgroup");

    snprintf(cgpath, sizeof(cgpath), "%s/%s", CGROUP_ROOT, CGROUP_NAME);
	if (mkdir(cgpath, 0755) == -1 && errno != EEXIST)
		die("mkdir");
}

int new_cgroup(char *name) {
	char cgpath[PATH_MAX];
    snprintf(cgpath, sizeof(cgpath), "%s/%s/%s", CGROUP_ROOT, CGROUP_NAME, name);
	if (mkdir(cgpath, 0755) == -1)
		die("mkdir");
	int fd = open(cgpath, O_DIRECTORY);
	if (fd < 0)
		die("cgroup open");
	return fd;
}

static void rm_cgroup_internal(char *path) {
    DIR *dir = opendir(path);
    if (!dir)
        die("opendir");

    struct dirent *de;
    while ((de = readdir(dir)) != NULL) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
            continue;

        char child[PATH_MAX];
        snprintf(child, sizeof(child), "%s/%s", path, de->d_name);

        struct stat st;
        if (lstat(child, &st) == -1)
            die("lstat");

        if (S_ISDIR(st.st_mode)) {
            rm_cgroup_internal(child);
            if (rmdir(child) == -1)
				die("rmdir");
        }
    }

    if (rmdir(path) == -1)
	    die("rmdir");
    closedir(dir);
}

void rm_cgroup(char *name) {
	char cgpath[PATH_MAX];
    snprintf(cgpath, sizeof(cgpath), "%s/%s/%s", CGROUP_ROOT, CGROUP_NAME, name);
	rm_cgroup_internal(cgpath);
}

void set_frozen_cgroup(char *name, int frozen) {
	char freeze_path[PATH_MAX];
	char *frozen_str = frozen ? "1" : "0";
    snprintf(freeze_path, sizeof(freeze_path), "%s/%s/%s/cgroup.freeze", CGROUP_ROOT, CGROUP_NAME, name);
	int fd = open(freeze_path, O_WRONLY);
	if (fd < 0)
		die("cgroup.freeze open");
	if (write(fd, frozen_str, 1) != 1)
		die("freeze write");
	close(fd);
}

void kill_cgroup(char *name) {
	char kill_path[PATH_MAX];
    snprintf(kill_path, sizeof(kill_path), "%s/%s/%s/cgroup.kill", CGROUP_ROOT, CGROUP_NAME, name);
	int fd = open(kill_path, O_WRONLY);
	if (fd < 0)
		die("cgroup.kill open");
	if (write(fd, "1", 1) != 1)
		die("kill write");
	close(fd);
}
