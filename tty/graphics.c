#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/kd.h>
#include <sys/ioctl.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <vt_number>\n", argv[0]);
        return 1;
    }

    int vt = atoi(argv[1]);
    char path[32];
    snprintf(path, sizeof(path), "/dev/tty%d", vt);

    int fd = open(path, O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    if (ioctl(fd, KDSETMODE, KD_GRAPHICS) < 0) {
        perror("KDSETMODE");
        close(fd);
        return 1;
    }

    close(fd);
    return 0;
}
