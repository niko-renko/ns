#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

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

#include "common.h"
#include "cmd.h"

#define DEV_PATH "/dev/input/event7"


void vt_switch(int tty0, int tty63) {
	// Freeze all

	if (ioctl(tty0, VT_ACTIVATE, 63) < 0)
		die("VT_ACTIVATE");
	
	if (ioctl(tty0, VT_WAITACTIVE, 63) < 0)
		die("VT_WAITACTIVE");

	if (ioctl(tty63, KDSETMODE, KD_TEXT) < 0)
		die("KD_TEXT");

	if (ioctl(tty63, KDSKBMODE, K_UNICODE) < 0)
		die("KDSKBMODE");
}

void kbd(int tty0, int tty63) {
	int fd = open(DEV_PATH, O_RDONLY);
	if (fd < 0)
		die("kbd open");

	struct input_event ev;
	int ctrl = 0, alt = 0, j = 0;
	
	while (1) {
		ssize_t n = read(fd, &ev, sizeof(ev));
		if (n != sizeof(ev))
			die("kbd read");

		if (ev.type != EV_KEY)
			continue;

		if (ev.code == KEY_LEFTCTRL || ev.code == KEY_RIGHTCTRL)
			ctrl = ev.value;
		else if (ev.code == KEY_LEFTALT || ev.code == KEY_RIGHTALT)
			alt = ev.value;
		else if (ev.code == KEY_J)
			j = ev.value;

		if (ctrl && alt && j)
			vt_switch(tty0, tty63);
	}
}

void bash(int tty0, int tty63) {
	dup2(tty63, STDIN_FILENO);
	dup2(tty63, STDOUT_FILENO);
	dup2(tty63, STDERR_FILENO);

	vt_switch(tty0, tty63);
	execl("/bin/bash", "bash", (char *)NULL);
}

int main(void) {
	setenv("PATH", "/bin:/usr/bin", 1);
	setenv("HOME", "/root", 1);

	if (setsid() < 0)
		die("setsid");

	int tty0 = open("/dev/tty0", O_RDWR);
	if (tty0 < 0)
		die("tty0 open");

	int tty63 = open("/dev/tty63", O_RDWR);
	if (tty63 < 0)
		die("tty63 open");

	pid_t pid;
	pid = fork();
	if (pid < 0)
		die("fork");
	if (pid == 0)
		bash(tty0, tty63);

	pid = fork();
	if (pid < 0)
		die("fork");
	if (pid == 0)
		kbd(tty0, tty63);

	pid = fork();
	if (pid < 0)
		die("fork");
	if (pid == 0)
		cmd(tty0);
		
	for (;;) pause();
}
