#ifndef CGROUP_H
#define CGROUP_H

#define CGROUP_ROOT "/sys/fs/cgroup"
#define CGROUP_NAME "initns"

void configure_cgroup(void);
int new_cgroup(char *);

#endif