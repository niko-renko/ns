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

	init_cgroup();
	spawn_kbd();
	spawn_sock_cmd();

	for (;;) pause();
}
