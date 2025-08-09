#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/input.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <linux/vt.h>
#include <linux/kd.h>
#include <signal.h>

void die(const char *msg) {
	perror(msg);
	exit(1);
}

int main(void) {
	int fd;

	if (setsid() < 0)
		die("setsid");

	fd = open("/dev/tty63", O_RDWR);
	if (fd < 0)
		die("tty63 open");
	if (ioctl(fd, TIOCSCTTY, 1) < 0)
		die("TIOCSCTTY");
	dup2(fd, STDIN_FILENO);
	dup2(fd, STDOUT_FILENO);
	dup2(fd, STDERR_FILENO);
	close(fd);

	fd = open("/dev/tty0", O_RDWR);
	if (fd < 0)
		die("tty0 open");
	if (ioctl(fd, VT_ACTIVATE, 63) < 0)
		die("VT_ACTIVATE");
	if (ioctl(fd, VT_WAITACTIVE, 63) < 0)
		die("VT_WAITACTIVE");
	close(fd);

	setenv("PATH", "/usr/bin:/bin", 1);
	setenv("HOME", "/root", 1);

	execl("/bin/bash", "bash", (char *)NULL);
}
