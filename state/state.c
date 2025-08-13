#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "state.h"

static pthread_key_t state_key;
static pthread_once_t key_once = PTHREAD_ONCE_INIT;

static void create_key(void) {
    pthread_key_create(&state_key, free);
}

void set_state(State *state) {
    pthread_once(&key_once, create_key);
    pthread_setspecific(state_key, state);
}

State *get_state(void) {
    return pthread_getspecific(state_key);
}
