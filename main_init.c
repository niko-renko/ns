#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>

#include "common.h"
#include "state/state.h"
#include "cmd/cmd.h"
#include "kbd/kbd.h"
#include "vt/vt.h"
#include "cgroup/cgroup.h"

int main(void) {
	State *state = init_state();
	set_state(state);

	clean_fds();
    int console = open("/dev/tty9", O_RDWR);
    if (console < 0)
        die("console open");
	dup2(console, STDOUT_FILENO);
	dup2(console, STDERR_FILENO);

	init_cgroup();
	spawn_kbd();
	spawn_sock_cmd();

	for (;;) pause();
}
