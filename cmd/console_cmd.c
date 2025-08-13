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

void accept_cmd(int, char *, int);

static void *console_cmd(void *arg) {
	char buf[256];
    int n;

	while ((n = read(0, buf, sizeof(buf) - 1)) > 0)
        accept_cmd(1, buf, n);
}

void spawn_console_cmd(void) {
    pthread_t console_cmd_t;
    if (pthread_create(&console_cmd_t, NULL, console_cmd, NULL) != 0)
        die("pthread_create");
    return;
}