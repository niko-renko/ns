#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/vt.h>

void die(const char *msg) {
	perror(msg);
	exit(1);
}

int main(int argc, char **argv) {
    	if (argc != 2) {
    	    fprintf(stderr, "Usage: %s <cgroup_name>\n", argv[0]);
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
    	snprintf(path, sizeof(path), "/sys/fs/cgroup/%s/cgroup.freeze", argv[1]);

    	fd = open(path, O_WRONLY);
    	if (fd < 0)
    	    die("open cgroup.freeze");

    	if (write(fd, "0", 1) != 1)
    	    die("write cgroup.freeze");

    	close(fd);
    	printf("Thawed cgroup: %s\n", argv[1]);
    	return 0;
}

