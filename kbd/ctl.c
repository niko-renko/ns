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

static int _denyfd;

static void clone_shell() {
    pid_t pid = fork();
    if (pid < 0)
        die("fork");
    if (pid > 0)
        return;
	clean_fds();

    int tty63 = open("/dev/tty63", O_RDWR | O_NOCTTY);
    if (tty63 < 0)
        die("tty63 open");
	dup2(tty63, STDOUT_FILENO);
	dup2(tty63, STDERR_FILENO);

	setenv("PATH", "/bin:/usr/bin", 1);
	setenv("HOME", "/root", 1);
	execl("/bin/bash", "bash", (char *)NULL);
}

static void handle_sigusr1(int signo) {
    (void)signo;
    if (ioctl(_denyfd, VT_RELDISP, 0) < 0)
        die("VT_RELDISP deny");
}

static pid_t clone_ctl() {
    pid_t pid = fork();
    if (pid < 0)
        die("fork");
    if (pid > 0)
        return pid;
	clean_fds();

	if (setsid() < 0)
		die("setsid");

    int tty63 = open("/dev/tty63", O_RDWR | O_NOCTTY);
    if (tty63 < 0)
        die("tty63 open");
    if (ioctl(tty63, TIOCSCTTY, (void *)1) < 0) 
        die("TIOCSCTTY");
	if (ioctl(tty63, KDSETMODE, KD_TEXT) < 0)
		die("KD_TEXT");
	if (ioctl(tty63, KDSKBMODE, K_UNICODE) < 0)
		die("KDSKBMODE");

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

	_denyfd = tty63;
    if (sigaction(SIGUSR1, &sa1, NULL) < 0)
        die("sigaction SIGUSR1");
	clone_shell();
	for (;;) pause();
}

void ctl(void) {
	// Freeze active
	State *state = get_state();
	pthread_mutex_lock(&state->lock);
	if (state->ctl)
        clone_pkill(state->ctl);
	state->ctl = clone_ctl();
	pthread_mutex_unlock(&state->lock);
	switch_vt(63);
}
