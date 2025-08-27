#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#include "common.h"
#include "state/state.h"
#include "cmd/cmd.h"
#include "kbd/kbd.h"
#include "cgroup/cgroup.h"

int main(void) {
	State *state = init_state();
	set_state(state);

	if (mkdir(ROOT, 0755) == -1)
        if (errno != EEXIST)
			die("ROOT mkdir");

	init_cgroup();
	spawn_kbd();
	spawn_sock_cmd();

	for (;;) pause();
}
