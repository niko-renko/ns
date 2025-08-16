#include <pthread.h>

#include "./state/state.h"
#include "./cmd/cmd.h"

int main(void) {
	State *state = init_state();
    set_state(state);
    cmd(0, 1);
}