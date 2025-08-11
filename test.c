#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#include <dirent.h>

#define INPUT_DIR "/dev/input"
#define EVENT_PREFIX "event"

static volatile int stop_flag = 0;
static pthread_t monitor_thread_id;

static void on_device_added(const char *devpath) {
    // Empty handler for device added event
    printf("added: %s\n", devpath);
}

static void on_device_removed(const char *devpath) {
    // Empty handler for device removed event
    printf("removed: %s\n", devpath);
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
}

static void *monitor_thread(void *arg) {
    int inotify_fd = inotify_init1(IN_NONBLOCK);
    if (inotify_fd < 0) return NULL;

    int wd = inotify_add_watch(inotify_fd, INPUT_DIR, IN_CREATE | IN_DELETE);
    if (wd < 0) {
        close(inotify_fd);
        return NULL;
    }

    scan_existing_devices();

    char buf[4096]
        __attribute__ ((aligned(__alignof__(struct inotify_event))));
    ssize_t len;

    while (!stop_flag) {
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

                if (event->mask & IN_CREATE) {
                    on_device_added(fullpath);
                }
                if (event->mask & IN_DELETE) {
                    on_device_removed(fullpath);
                }
            }
            ptr += sizeof(struct inotify_event) + event->len;
        }
    }

    inotify_rm_watch(inotify_fd, wd);
    close(inotify_fd);
    return NULL;
}

int main(void) {
    if (pthread_create(&monitor_thread_id, NULL, monitor_thread, NULL) != 0) {
        perror("pthread_create");
        return 1;
    }

    // Example: run for 30 seconds then stop
    sleep(30);
    stop_flag = 1;
    pthread_join(monitor_thread_id, NULL);

    return 0;
}
