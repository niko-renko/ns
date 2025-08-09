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

#define DEV_PATH "/dev/input/event7"
				     
void die(const char *msg) {
	perror(msg);
	exit(1);
}

pid_t handle(pid_t existing) {
	// Freeze all
	pid_t pid;

	if (existing == 0 || kill(existing, 0) != 0)
		pid = fork();
	else
		pid = existing;

	if (pid < 0)
		die("handle: fork");
	if (pid > 0)
		return pid;

	if (setsid() < 0)
		die("handle: setsid");

	int fd = open("/dev/tty63", O_RDWR);
	dup2(fd, STDIN_FILENO);
	dup2(fd, STDOUT_FILENO);
	dup2(fd, STDERR_FILENO);

	if (fd < 0)
		die("tty63 open");

	if (ioctl(fd, TIOCSCTTY, 1) < 0)
		die("TIOCSCTTY, 1");

	if (ioctl(fd, VT_ACTIVATE, 63) < 0)
		die("VT_ACTIVATE");
	
	if (ioctl(fd, VT_WAITACTIVE, 63) < 0)
		die("VT_WAITACTIVE");

	struct vt_mode mode = {0};
	mode.mode = VT_AUTO;
	
	if (ioctl(fd, VT_SETMODE, &mode) < 0)
		die("VT_SETMODE");

	if (ioctl(fd, KDSETMODE, KD_TEXT) < 0)
		die("KD_TEXT");

	if (ioctl(fd, KDSKBMODE, K_UNICODE) < 0)
		die("KDSKBMODE");

	execl("/bin/bash", "bash", (char *)NULL);
}

int main(void) {
	pid_t bash = 0;
	int fd = open(DEV_PATH, O_RDONLY);
	if (fd < 0)
		die("kbd open");
	
	struct input_event ev;
	int ctrl = 0, alt = 0, l = 0;
	
	while (1) {
		ssize_t n = read(fd, &ev, sizeof(ev));
		if (n != sizeof(ev))
			die("kbd read");

		if (ev.type == EV_KEY) {
			if (ev.code == KEY_LEFTCTRL || ev.code == KEY_RIGHTCTRL)
				ctrl = ev.value;
			else if (ev.code == KEY_LEFTALT || ev.code == KEY_RIGHTALT)
				alt = ev.value;
			else if (ev.code == KEY_L)
				l = ev.value;
			if (ctrl && alt && l)
				bash = handle(bash);
		}
	}
}
