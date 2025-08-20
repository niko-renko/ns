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
#include "../state/state.h"
#include "../ctl/ctl.h"
#include "../cgroup/cgroup.h"

struct seq_listener_args {
    State *state;
    char device_path[PATH_MAX];
};

static void on_seq() {
    start_ctl();

    State *state = get_state();
	pthread_mutex_lock(&state->lock);
    if (state->active == -1)
        goto unlock;
    char *instance = state->instances[state->active];
    set_frozen_cgroup(instance, 1);
unlock:
	pthread_mutex_unlock(&state->lock);
}

static void *seq_listener(void *arg) {
    struct seq_listener_args *args = arg;
    set_state(args->state);

    int fd = open(args->device_path, O_RDONLY | O_NONBLOCK);
    free(args);
    if (fd < 0)
        thread_die("open_device");

    struct input_event ev;
    int ctrl = 0, alt = 0;

    while (1) {
        ssize_t n = read(fd, &ev, sizeof(ev));

        if (n < 0 && !(errno == EAGAIN || errno == EWOULDBLOCK))
            thread_die("device read");
        if (n != (ssize_t)sizeof(ev) || ev.type != EV_KEY)
            continue;

        if (ev.code == KEY_LEFTCTRL || ev.code == KEY_RIGHTCTRL)
            ctrl = ev.value;
        else if (ev.code == KEY_LEFTALT || ev.code == KEY_RIGHTALT)
            alt = ev.value;
        else if (ev.code == KEY_J && ev.value == 1)
            if (ctrl && alt)
                on_seq();
    }

    close(fd);
    return NULL;
}

void spawn_seq_listener(char *devpath) {
    pthread_t seq_listener_t;
    struct seq_listener_args *args = malloc(sizeof(*args));
    args->state = get_state();
    strncpy(args->device_path, devpath, sizeof(args->device_path) - 1);
    args->device_path[sizeof(args->device_path) - 1] = '\0';
    if (pthread_create(&seq_listener_t, NULL, seq_listener, args) != 0)
        die("pthread_create");
    return;
}