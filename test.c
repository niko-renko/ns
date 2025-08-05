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
	//if (mkdir("/sys/fs/cgroup/test", 0755) == -1 && errno != EEXIST)
	//	die("mkdir");
	
	//if (mount("cgroup", "/sys/fs/cgroup", "cgroup2", 0, NULL) < 0)
	//	die("cgroup");
	const char *cgroup_path = "/sys/fs/cgroup/test";

    	// Create cgroup directory
    	//if (mkdir(cgroup_path, 0755) == -1) {
    	//    perror("mkdir");
    	//    return 1;
    	//}
	
	char procs_path[256];
    	snprintf(procs_path, sizeof(procs_path), "%s/cgroup.procs", cgroup_path);
    	int fd = open(procs_path, O_WRONLY);
    	if (fd == -1) {
    	    perror("open");
    	    return 1;
    	}
	
	// Write current PID to cgroup.procs
    	char pid_str[32];
    	snprintf(pid_str, sizeof(pid_str), "%d\n", getpid());
    	if (write(fd, pid_str, strlen(pid_str)) == -1) {
    	    perror("write");
    	    close(fd);
    	    return 1;
    	}

    	close(fd);
	while (1) {
    	    printf("Message\n");
    	    sleep(1);
    	}
    	//pause(); // Keep process alive for observation
    	return 0;
}
