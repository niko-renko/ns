#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

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
#include "cmd/cmd.h"
#include "kbd/kbd.h"

int main(void) {
	pid_t pid;

	pid = fork();
	if (pid < 0)
		die("fork");
	if (pid == 0)
		kbd();

	pid = fork();
	if (pid < 0)
		die("fork");
	if (pid == 0)
		sock_cmd();
		
	for (;;) pause();
}
