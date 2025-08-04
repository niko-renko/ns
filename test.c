#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/mount.h>

void die(const char *msg) {
	perror(msg);
	exit(1);
}

int main() {

	if (mount("cgroup", "/sys/fs/cgroup", "cgroup2", 0, NULL) < 0)
		die("cgroup");
	if (mkdir("/sys/fs/cgroup/test", 0755) == -1 && errno != EEXIST)
		die("mkdir");
}
