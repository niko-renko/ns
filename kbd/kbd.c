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

#include "../state/state.h"
#include "../common.h"

#define INPUT_DIR "/dev/input"
#define EVENT_PREFIX "event"

static void spawn_seq_listener(char *);

static int is_kbd(const char *path) {
    int fd = open(path, O_RDONLY | O_NONBLOCK);
    if (fd < 0) return 0;

    unsigned long evbit[(EV_MAX + 7) / 8] = {0};
    unsigned long keybit[(KEY_MAX + 7) / 8] = {0};

    if (ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), evbit) < 0) {
        close(fd);
        return 0;
    }

    if (!(evbit[EV_KEY / 8] & (1 << (EV_KEY % 8))) ||
        !(evbit[EV_SYN / 8] & (1 << (EV_SYN % 8)))) {
        close(fd);
        return 0;
    }

    if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybit)), keybit) < 0) {
        close(fd);
        return 0;
    }

    int count = 0;
    for (int code = 1; code <= 255; code++) {
        if (keybit[code / 8] & (1 << (code % 8))) count++;
        if (count > 15) break;
    }

    close(fd);
    return count > 15;
}

static void on_device_added(const char *devpath) {
    if (!is_kbd(devpath))
        return;
    printf("added: %s\n", devpath);
    spawn_seq_listener(devpath);
}

static int is_event_device(const char *name) {
    return strncmp(name, EVENT_PREFIX, strlen(EVENT_PREFIX)) == 0;
}

static void scan_existing_devices(void) {
    DIR *dir = opendir(INPUT_DIR);
    if (!dir) return;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (is_event_device(entry->d_name)) {
            char fullpath[PATH_MAX];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", INPUT_DIR, entry->d_name);
            on_device_added(fullpath);
        }
    }
    closedir(dir);
    return;
}

static void *kbd(void *arg) {
    State *state = arg;
    set_state(state);
    int inotify_fd = inotify_init1(IN_NONBLOCK);
    if (inotify_fd < 0) return NULL;

    int wd = inotify_add_watch(inotify_fd, INPUT_DIR, IN_CREATE);
    if (wd < 0) {
        close(inotify_fd);
        return NULL;
    }

    scan_existing_devices();

    char buf[4096]
        __attribute__ ((aligned(__alignof__(struct inotify_event))));
    ssize_t len;

    while (1) {
        len = read(inotify_fd, buf, sizeof(buf));
        if (len < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                usleep(200000);
                continue;
            } else break;
        }
        if (len == 0) {
            usleep(200000);
            continue;
        }

        for (char *ptr = buf; ptr < buf + len; ) {
            struct inotify_event *event = (struct inotify_event *) ptr;
            if (is_event_device(event->name)) {
                char fullpath[PATH_MAX];
                snprintf(fullpath, sizeof(fullpath), "%s/%s", INPUT_DIR, event->name);
                if (event->mask & IN_CREATE)
                    on_device_added(fullpath);
            }
            ptr += sizeof(struct inotify_event) + event->len;
        }
    }

    inotify_rm_watch(inotify_fd, wd);
    close(inotify_fd);
    return NULL;
}

void spawn_kbd(State *state) {
    pthread_t kbd_t;
    if (pthread_create(&kbd_t, NULL, kbd, state) != 0)
        die("pthread_create");
    return;
}