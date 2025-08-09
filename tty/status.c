#include <linux/vt.h>
#include <linux/kd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void check_tty(int num) {
    char path[32];
    snprintf(path, sizeof(path), "/dev/tty%d", num);

    int fd = open(path, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        perror("open");
        exit(1);
    }

    struct vt_mode mode;
    if (ioctl(fd, VT_GETMODE, &mode) < 0) {
        perror("VT_GETMODE");
    } else {
        switch (mode.mode) {
            case VT_AUTO:   printf("Attachment mode: AUTO\n"); break;
            case VT_PROCESS:printf("Attachment mode: PROCESS\n"); break;
            default:        printf("Attachment mode: UNKNOWN(%d)\n", mode.mode); break;
        }
    }

    int kd_mode;
    if (ioctl(fd, KDGETMODE, &kd_mode) < 0) {
        perror("KDGETMODE");
    } else {
        switch (kd_mode) {
            case KD_TEXT:    printf("Render mode: TEXT\n"); break;
            case KD_GRAPHICS:printf("Render mode: GRAPHICS\n"); break;
            default:         printf("Render mode: UNKNOWN(%d)\n", kd_mode); break;
        }
    }

    int kb_mode;
    if (ioctl(fd, KDGKBMODE, &kb_mode) < 0) {
        perror("KDGKBMODE");
    } else {
        switch (kb_mode) {
            case K_RAW:      printf("Keyboard mode: RAW\n"); break;
            case K_XLATE:    printf("Keyboard mode: XLATE\n"); break;
            case K_MEDIUMRAW:printf("Keyboard mode: MEDIUMRAW\n"); break;
            case K_UNICODE:  printf("Keyboard mode: UNICODE\n"); break;
            case K_OFF:  printf("Keyboard mode: OFF\n"); break;
            default:         printf("Keyboard mode: UNKNOWN(%d)\n", kb_mode); break;
        }
    }

    close(fd);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <tty_number>\n", argv[0]);
        return 1;
    }
    check_tty(atoi(argv[1]));
    return 0;
}
