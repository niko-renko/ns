#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>

#include "common.h"
#include "state/state.h"
#include "cmd/cmd.h"
#include "kbd/kbd.h"
#include "cgroup/cgroup.h"

int main(void) {
	State *state = init_state();
	set_state(state);

	clean_fds();
	int log = open("/root/Code/log", O_WRONLY | O_APPEND | O_CREAT);
	dup2(log, STDOUT_FILENO);
	dup2(log, STDERR_FILENO);

	init_cgroup();
	spawn_kbd();
	spawn_sock_cmd();

	for (;;) pause();
}
