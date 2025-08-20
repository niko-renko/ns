#ifndef STATE_H
#define STATE_H

typedef struct state {
    pthread_mutex_t lock;
    pid_t ctl;
    int instances_n;
    char **instances;
    int active;
} State;

State *init_state(void);
void set_state(State *);
State *get_state(void);

int get_instance(State *, const char *);
int add_instance(State *, const char *);

#endif