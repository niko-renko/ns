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

pid_t spawn_shell() {
    pid_t pid = fork();
    if (pid < 0)
        die("fork");
    if (pid > 0)
        return pid;
	clean_fds();

    int tty0 = open("/dev/tty0", O_RDWR);
    if (tty0 < 0)
        die("tty0 open");
	if (ioctl(tty0, VT_ACTIVATE, 63) < 0)
		die("VT_ACTIVATE");
	if (ioctl(tty0, VT_WAITACTIVE, 63) < 0)
		die("VT_WAITACTIVE");
    close(tty0);

    int tty63 = open("/dev/tty63", O_RDWR);
    if (tty63 < 0)
        die("tty63 open");
	if (ioctl(tty63, KDSETMODE, KD_TEXT) < 0)
		die("KD_TEXT");
	if (ioctl(tty63, KDSKBMODE, K_UNICODE) < 0)
		die("KDSKBMODE");

	dup2(tty63, STDIN_FILENO);
	dup2(tty63, STDOUT_FILENO);
	dup2(tty63, STDERR_FILENO);
    close(tty63);

	if (setsid() < 0)
		die("setsid");

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

	return;
}