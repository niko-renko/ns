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
	int allow = state->allow;
	pthread_mutex_unlock(&state->lock);

    if (ioctl(tty63, VT_RELDISP, allow) < 0) {
        perror("VT_RELDISP deny");
    } else {
        printf("Denied VT release request\n");
    }
}

static void handle_sigusr2(int signo) {
    (void)signo;
    if (ioctl(tty63, VT_RELDISP, VT_ACKACQ) < 0) {
        perror("VT_RELDISP ackacq");
    } else {
        printf("Acknowledged VT acquire\n");
    }
}

pid_t spawn_shell() {
    pid_t pid = fork();
    if (pid < 0)
        die("fork");
    if (pid > 0)
        return pid;
	clean_fds();

	if (setsid() < 0)
		die("setsid");

    int tty0 = open("/dev/tty0", O_RDWR);
    if (tty0 < 0)
        die("tty0 open");
	if (ioctl(tty0, VT_ACTIVATE, 63) < 0)
		die("VT_ACTIVATE");
	if (ioctl(tty0, VT_WAITACTIVE, 63) < 0)
		die("VT_WAITACTIVE");
    close(tty0);

    int tty63 = open("/dev/tty63", O_RDWR | O_NOCTTY);
    if (tty63 < 0)
        die("tty63 open");

	dup2(tty63, STDOUT_FILENO);
	dup2(tty63, STDERR_FILENO);

    if (ioctl(tty63, TIOCSCTTY, (void *)1) < 0) 
        die("TIOCSCTTY");
	if (ioctl(tty63, KDSETMODE, KD_TEXT) < 0)
		die("KD_TEXT");
	if (ioctl(tty63, KDSKBMODE, K_UNICODE) < 0)
		die("KDSKBMODE");

	setenv("PATH", "/bin:/usr/bin", 1);
	setenv("HOME", "/root", 1);
	execl("/bin/bash", "bash", (char *)NULL);
}

void ctl(void) {
	// Freeze active
	State *state = get_state();
	pthread_mutex_lock(&state->lock);
	if (state->ctl == 1) {
		pthread_mutex_unlock(&state->lock);
		return;
	}
	state->ctl = 1;
	pthread_mutex_unlock(&state->lock);

	pid_t pid = spawn_shell();

    tty63 = open("/dev/tty63", O_RDWR | O_NOCTTY);

    struct vt_mode mode = {0};
    mode.mode = VT_PROCESS;
    mode.relsig = SIGUSR1;
    mode.acqsig = SIGUSR2;

    if (ioctl(tty63, VT_SETMODE, &mode) < 0) {
        close(tty63);
        die("VT_SETMODE");
    }

    struct sigaction sa1 = {0}, sa2 = {0};
    sa1.sa_handler = handle_sigusr1;
    sa2.sa_handler = handle_sigusr2;
    sigemptyset(&sa1.sa_mask);
    sigemptyset(&sa2.sa_mask);
    sa1.sa_flags = 0;
    sa2.sa_flags = 0;

    if (sigaction(SIGUSR1, &sa1, NULL) < 0) {
        close(tty63);
        die("sigaction SIGUSR1");
    }
    if (sigaction(SIGUSR2, &sa2, NULL) < 0) {
        close(tty63);
        die("sigaction SIGUSR2");
    }

	return;
}
