#ifndef COMMON_H
#define COMMON_H

#define ROOT "/var/lib/initns"

void die(const char *);
void thread_die(const char *);
void clean_fds(void);

#endif