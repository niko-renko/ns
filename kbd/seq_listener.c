#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#include <linux/input.h>
#include <dirent.h>

#include "../common.h"
#include "ctl/ctl.h"

struct seq_listener_args {
    char device_path[PATH_MAX];
};

static void handle() {
    ctl();
}

static void *seq_listener(void *arg) {
    struct seq_listener_args *args = arg;
    int fd = open(args->device_path, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("open device");
        free(args);
        return NULL;
    }

    struct input_event ev;
    int ctrl = 0, alt = 0;

    while (1) {
        ssize_t n = read(fd, &ev, sizeof(ev));
        if (n == (ssize_t)sizeof(ev)) {
            if (ev.type == EV_KEY) {
                if (ev.code == KEY_LEFTCTRL || ev.code == KEY_RIGHTCTRL)
                    ctrl = ev.value;
                else if (ev.code == KEY_LEFTALT || ev.code == KEY_RIGHTALT)
                    alt = ev.value;
                else if (ev.code == KEY_J && ev.value == 1)
                    if (ctrl && alt)
                        handle();
            }
        } else if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                usleep(10000);
                continue;
            } else {
                perror("read");
                break;
            }
        }
    }

    close(fd);
    free(args);
    return NULL;
}

void spawn_seq_listener(char *devpath) {
    pthread_t seq_listener_t;
    struct seq_listener_args *args = malloc(sizeof(*args));
    strncpy(args->device_path, devpath, sizeof(args->device_path) - 1);
    args->device_path[sizeof(args->device_path) - 1] = '\0';
    if (pthread_create(&seq_listener_t, NULL, seq_listener, args) != 0)
        die("pthread_create");
    return;
}