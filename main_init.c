#include <pthread.h>
#include <unistd.h>

#include "common.h"
#include "state/state.h"
#include "cmd/cmd.h"
#include "kbd/kbd.h"
#include "vt/vt.h"
#include "cgroup/cgroup.h"

int main(void) {
	State state = {
		.lock = PTHREAD_MUTEX_INITIALIZER,
		.ctl = 0
	};

	set_state(&state);

	configure_vt();
	configure_cgroup();

	spawn_kbd();
	spawn_sock_cmd();

	for (;;) pause();
}
