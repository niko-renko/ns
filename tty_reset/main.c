#include <linux/vt.h>
#include <linux/kd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void reset_tty(int num) {
    char path[32];
    snprintf(path, sizeof(path), "/dev/tty%d", num);

    int fd = open(path, O_RDWR);
    if (fd < 0) {
        perror("open");
        exit(1);
    }

    if (ioctl(fd, TIOCSCTTY, 1) < 0) {
        perror("TIOCSCTTY");
        close(fd);
        exit(1);
    }

    struct vt_mode mode = {0};
    mode.mode = VT_AUTO;

    if (ioctl(fd, VT_SETMODE, &mode) < 0) {
        perror("VT_SETMODE");
        close(fd);
        exit(1);
    }

    if (ioctl(fd, KDSETMODE, KD_TEXT) < 0) {
        perror("KDSETMODE");
        close(fd);
        exit(1);
    }

    if (ioctl(fd, KDSKBMODE, K_UNICODE) < 0) {
        perror("KDSKBMODE");
        close(fd);
        exit(1);
    }

    close(fd);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <tty_number>\n", argv[0]);
        return 1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }
    if (pid > 0)
	    return 0;

    if (setsid() < 0) {
        perror("setsid");
        exit(1);
    }
    reset_tty(atoi(argv[1]));
    exit(0);

    return 0;
}

