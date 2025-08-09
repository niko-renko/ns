#define _GNU_SOURCE
#include <fcntl.h>
#include <linux/vt.h>
#include <linux/kd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/select.h>
#include <errno.h>

int main() {
    const char *console = "/dev/console";
    const char *tty8 = "/dev/tty8";

    int cons_fd = open(console, O_RDWR | O_NONBLOCK);
    if (cons_fd < 0) {
        perror("open /dev/console");
        exit(1);
    }

    struct vt_stat vts;
    if (ioctl(cons_fd, VT_GETSTATE, &vts) < 0) {
        perror("VT_GETSTATE");
        close(cons_fd);
        exit(1);
    }

    if (vts.v_active == 8) {
        goto print_message;
    }

    if (ioctl(cons_fd, VT_WAITACTIVE, 8) < 0) {
        perror("VT_WAITACTIVE");
        close(cons_fd);
        exit(1);
    }

print_message:
    int tty_fd = open(tty8, O_WRONLY | O_NOCTTY);
    if (tty_fd < 0) {
        perror("open /dev/tty8");
        close(cons_fd);
        exit(1);
    }

    const char *msg = "tty8 is now active\n";
    write(tty_fd, msg, strlen(msg));

    close(tty_fd);
    close(cons_fd);
    return 0;
}
