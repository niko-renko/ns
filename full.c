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

void handle(void) {
	// Freeze all
	int fd;

	fd = open("/dev/tty0", O_RDWR);
	if (fd < 0)
		die("tty0 open");

	if (ioctl(fd, VT_ACTIVATE, 63) < 0)
		die("VT_ACTIVATE");
	
	if (ioctl(fd, VT_WAITACTIVE, 63) < 0)
		die("VT_WAITACTIVE");
	close(fd);

	fd = open("/dev/tty63", O_RDWR);
	if (fd < 0)
		die("tty63 open");

	if (ioctl(fd, KDSETMODE, KD_TEXT) < 0)
		die("KD_TEXT");

	if (ioctl(fd, KDSKBMODE, K_UNICODE) < 0)
		die("KDSKBMODE");
	close(fd);
}

int main(void) {
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
			else if (ev.code == KEY_J)
				l = ev.value;
			if (ctrl && alt && l)
				handle();
		}
	}
}
