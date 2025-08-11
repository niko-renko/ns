#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <errno.h>
#include <string.h>
#include <poll.h>

#include "../common.h"

#define DEVICE_PATH "/dev/input/event0"

void ctl(void);

int kbd(void) {
    int fd;
    struct input_event ev;
    struct pollfd pfd;
	int ctrl, alt, j;

    for (;;) {
        fd = open(DEVICE_PATH, O_RDONLY);
        if (fd < 0) {
            perror("open");
            sleep(1);
            continue;
        }

        pfd.fd = fd;
        pfd.events = POLLIN;

        for (;;) {
            int ret = poll(&pfd, 1, -1);
            if (ret < 0)
                break;

            if (!(pfd.revents & POLLIN))
				break;

            ssize_t n = read(fd, &ev, sizeof(ev));
            if (n != sizeof(ev))
				break;

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

        close(fd);
    }
}