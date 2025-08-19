#ifndef COMMON_H
#define COMMON_H

void die(const char *);
void clean_fds(void);
void clone_pkill(int);
void switch_vt(int);

#endif