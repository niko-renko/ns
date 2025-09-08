#ifndef COMMON_H
#define COMMON_H

#define ROOT "/var/lib/initns"
#define SOCK_PATH "/run/initns.sock"

void die(const char *);
void clean_fds(void);

#endif
