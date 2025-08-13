#include <pthread.h>

#include "./state/state.h"
#include "./cmd/cmd.h"

int main(void) {
	State state = {
		.lock = PTHREAD_MUTEX_INITIALIZER,
		.ctl = -1,
		.allow = 0
	};
    set_state(&state);
    cmd(0, 1);
}