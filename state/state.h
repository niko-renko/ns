#ifndef STATE_H
#define STATE_H

typedef struct state {
    pthread_mutex_t lock;
    int ctl;
} State;

void set_state(State *);
State *get_state(void);

#endif