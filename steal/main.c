#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <tty number>\n", argv[0]);
        exit(1);
    }

    char path[64];
    snprintf(path, sizeof(path), "/dev/tty%s", argv[1]);

    pid_t pid = fork();
    if (pid < 0) return 1;
    if (pid > 0) return 0;

    if (setsid() < 0) {
        perror("setsid");
        exit(1);
    }

    int fd = open(path, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        perror("open");
        exit(1);
    }

    if (ioctl(fd, TIOCSCTTY, (void *)1) < 0) {
        perror("ioctl TIOCSCTTY");
        close(fd);
        exit(1);
    }

    printf("Successfully stole %s\n", path);

    // Optional: test write
    write(fd, "Terminal stolen\n", 16);

    // Keep process alive to hold control
    // pause();

    close(fd);
    return 0;
}
