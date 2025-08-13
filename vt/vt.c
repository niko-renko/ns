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

    if (ioctl(tty63, VT_RELDISP, allow) < 0)
        die("VT_RELDISP deny");
}

void configure_vt(void) {
    tty63 = open("/dev/tty63", O_RDWR | O_NOCTTY);

    struct vt_mode mode = {0};
    mode.mode = VT_PROCESS;
    mode.relsig = SIGUSR1;
    mode.acqsig = SIGWINCH;

    if (ioctl(tty63, VT_SETMODE, &mode) < 0)
        die("VT_SETMODE");

    struct sigaction sa1 = {0};
    sa1.sa_handler = handle_sigusr1;
    sigemptyset(&sa1.sa_mask);
    sa1.sa_flags = 0;

    if (sigaction(SIGUSR1, &sa1, NULL) < 0)
        die("sigaction SIGUSR1");
}
