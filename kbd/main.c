#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/input.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <linux/vt.h>

#define DEV_PATH "/dev/input/event7" // adjust to your keyboard event device
				     
void handle(void) {
    int fd = open("/dev/tty0", O_RDWR);
    if (fd < 0) {
        perror("open");
        exit(1);
    }
    if (ioctl(fd, VT_ACTIVATE, 9) < 0) {
        perror("VT_ACTIVATE");
        close(fd);
        exit(1);
    }
    if (ioctl(fd, VT_WAITACTIVE, 9) < 0) {
        perror("VT_WAITACTIVE");
        close(fd);
        exit(1);
    }
    close(fd);
}

int main(void) {
    int fd = open(DEV_PATH, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    struct input_event ev;
    int ctrl = 0, alt = 0, l = 0;

    while (1) {
        ssize_t n = read(fd, &ev, sizeof(ev));
        if (n != sizeof(ev)) {
            perror("read");
            return 1;
        }
        if (ev.type == EV_KEY) {
            if (ev.code == KEY_LEFTCTRL || ev.code == KEY_RIGHTCTRL)
                ctrl = ev.value;
            else if (ev.code == KEY_LEFTALT || ev.code == KEY_RIGHTALT)
                alt = ev.value;
            else if (ev.code == KEY_L)
                l = ev.value;

            if (ctrl && alt && l) {
		    handle();
            }
        }
    }
}
