#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <linux/input.h>
#include <linux/kd.h>
#include <linux/sched.h>
#include <linux/vt.h>

#include <sys/mount.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>

void die(const char *msg) {
    perror(msg);
    exit(1);
}

void thread_die(const char *msg) {
    perror(msg);
    pthread_exit(NULL);
}

void clean_fds(void) {
    for (int fd = 0; fd < sysconf(_SC_OPEN_MAX); fd++)
        close(fd);
}