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

#include "common.h"
#include "state/state.h"
#include "cmd/cmd.h"
#include "kbd/kbd.h"

int main(void) {
	State state = {
		.lock = PTHREAD_MUTEX_INITIALIZER,
		.ctl = 0,
		.allow = 0
	};

	spawn_kbd(&state);
	spawn_sock_cmd(&state);
		
	for (;;) pause();
}
