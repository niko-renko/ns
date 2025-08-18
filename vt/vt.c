#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>

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
#include "../state/state.h"

static int tty63;

static void handle_sigusr1(int signo) {
    (void)signo;
	State *state = get_state();
	pthread_mutex_lock(&state->lock);
	int allow = !state->ctl;
	pthread_mutex_unlock(&state->lock);

    // If the controlling terminal dies, you lose the rights to do RELDISP
    if (ioctl(tty63, VT_RELDISP, 1) < 0)
        die("VT_RELDISP deny");
}

void set_vt_mode(void) {
    if (tty63 == NULL)
        tty63 = open("/dev/tty63", O_RDWR | O_NOCTTY);

    struct vt_mode mode = {0};
    mode.mode = VT_PROCESS;
    mode.relsig = SIGUSR1;
    mode.acqsig = SIGWINCH;

    if (ioctl(tty63, TIOCSCTTY, (void *)1) < 0) 
        die("TIOCSCTTY");
    if (ioctl(tty63, VT_SETMODE, &mode) < 0)
        die("VT_SETMODE");
}

void set_sigaction(void) {
    if (tty63 == NULL)
        tty63 = open("/dev/tty63", O_RDWR | O_NOCTTY);

    struct sigaction sa1 = {0};
    sa1.sa_handler = handle_sigusr1;
    sigemptyset(&sa1.sa_mask);
    sa1.sa_flags = 0;

    if (sigaction(SIGUSR1, &sa1, NULL) < 0)
        die("sigaction SIGUSR1");
}

void switch_vt(int vt) {
	int tty0 = open("/dev/tty0", O_RDWR);
	if (tty0 < 0)
		die("tty0 open");
    if (ioctl(tty0, VT_ACTIVATE, vt) < 0)
    	die("VT_ACTIVATE");
    while (ioctl(tty0, VT_WAITACTIVE, vt) < 0) {
        if (errno == EINTR) continue;
        perror("VT_WAITACTIVE");
        break;
    }
	close(tty0);
}
