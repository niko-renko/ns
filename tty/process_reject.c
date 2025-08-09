#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/vt.h>
#include <signal.h>

static int vt_fd;

static void handle_sigusr1(int signo) {
    (void)signo;
    // Deny VT release request
    if (ioctl(vt_fd, VT_RELDISP, 0) < 0) {
        perror("VT_RELDISP deny");
    } else {
        printf("Denied VT release request\n");
    }
}

static void handle_sigusr2(int signo) {
    (void)signo;
    // Acknowledge VT acquire (optional)
    if (ioctl(vt_fd, VT_RELDISP, VT_ACKACQ) < 0) {
        perror("VT_RELDISP ackacq");
    } else {
        printf("Acknowledged VT acquire\n");
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <vt-number>\n", argv[0]);
        return 1;
    }

    int vt = atoi(argv[1]);
    if (vt <= 0) {
        fprintf(stderr, "Invalid VT number\n");
        return 1;
    }

    char tty_path[64];
    snprintf(tty_path, sizeof(tty_path), "/dev/tty%d", vt);

    vt_fd = open(tty_path, O_RDWR | O_NOCTTY);
    if (vt_fd < 0) {
        perror("open vt");
        return 1;
    }

    struct vt_mode mode = {0};
    mode.mode = VT_PROCESS;
    mode.relsig = SIGUSR1;
    mode.acqsig = SIGUSR2;

    if (ioctl(vt_fd, VT_SETMODE, &mode) < 0) {
        perror("VT_SETMODE");
        close(vt_fd);
        return 1;
    }

    struct sigaction sa1 = {0}, sa2 = {0};
    sa1.sa_handler = handle_sigusr1;
    sa2.sa_handler = handle_sigusr2;
    sigemptyset(&sa1.sa_mask);
    sigemptyset(&sa2.sa_mask);
    sa1.sa_flags = 0;
    sa2.sa_flags = 0;

    if (sigaction(SIGUSR1, &sa1, NULL) < 0) {
        perror("sigaction SIGUSR1");
        close(vt_fd);
        return 1;
    }
    if (sigaction(SIGUSR2, &sa2, NULL) < 0) {
        perror("sigaction SIGUSR2");
        close(vt_fd);
        return 1;
    }

    printf("VT %d set to VT_PROCESS mode, ignoring release requests\n", vt);

    close(vt_fd);
    // Keep process alive to receive signals
    while (1) {
        pause();
    }

    close(vt_fd);
    return 0;
}

