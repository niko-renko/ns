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

#include "../common.h"

#define DEV_PATH "/dev/input/event7"

void ctl(void);

void kbd(void) {
	// Make this the only way to get back
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
			ctl();
	}
}