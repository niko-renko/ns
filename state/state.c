#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../common.h"
#include "state.h"

static pthread_key_t state_key;
static pthread_once_t key_once = PTHREAD_ONCE_INIT;

static void create_key(void) {
    pthread_key_create(&state_key, NULL);
}

State *init_state(void) {
    State *state = malloc(sizeof(State));
    if(pthread_mutex_init(&state->lock, NULL) != 0)
        die("mutex_init");
    state->ctl = 0;
    state->instances_n = 0;
    state->instances = malloc(128 * sizeof(char *));
    state->active = -1;
    return state;
}

void set_state(State *state) {
    pthread_once(&key_once, create_key);
    pthread_setspecific(state_key, state);
}

State *get_state(void) {
    return pthread_getspecific(state_key);
}

int get_instance(State *state, const char *name) {
    for (int i = 0; i < state->instances_n; i++) {
        if (strcmp(state->instances[i], name) == 0)
            return i;
    }
    return -1;
}

int add_instance(State *state, const char *name) {
    if (state->instances_n >= 128)
        return -1;

    state->instances[state->instances_n] = malloc(strlen(name) + 1);
    strcpy(state->instances[state->instances_n], name);
    state->instances_n++;
    return state->instances_n - 1;
}