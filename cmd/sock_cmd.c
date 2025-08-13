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
#include "../set/set.h"
#include "../state/state.h"

#define SOCK_PATH "/run/initns.sock"

void cmd(int, int);

static void *sock_cmd(void *arg) {
	State *state = arg;
    set_state(state);
	int fd, cfd;
	struct sockaddr_un addr;
	
	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd == -1)
		die("socket");
	
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, SOCK_PATH, sizeof(addr.sun_path) - 1);
	unlink(addr.sun_path);
	
	if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
		die("bind");
	
	if (listen(fd, 5) == -1)
		die("listen");

	for (;;) {
		cfd = accept(fd, NULL, NULL);

		if (cfd == -1) {
			if (errno == EINTR) continue;
			perror("accept");
			break;
		}
		
		cmd(cfd, cfd);
		close(cfd);
	}

	close(fd);
	unlink(SOCK_PATH);
}

void spawn_sock_cmd(State *state) {
    pthread_t sock_cmd_t;
    if (pthread_create(&sock_cmd_t, NULL, sock_cmd, state) != 0)
        die("pthread_create");
    return;
}