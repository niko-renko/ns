#ifndef COMMON_H
#define COMMON_H

typedef struct state {
    pthread_mutex_t lock;
    int ctl;
} State;

void die(const char *);
void clean_fds(void);

#endif