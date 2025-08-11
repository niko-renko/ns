#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <errno.h>
#include <string.h>
#include <poll.h>
#include <libudev.h>

#include "../common.h"

void ctl(void);

char **find_all_kbds(size_t *count) {
    struct udev *udev = udev_new();
    if (!udev) {
        fprintf(stderr, "Failed to create udev\n");
        return NULL;
    }

    struct udev_enumerate *enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "input");
    udev_enumerate_scan_devices(enumerate);

    struct udev_list_entry *devices = udev_enumerate_get_list_entry(enumerate);
    struct udev_list_entry *entry;

    size_t capacity = 8;
    size_t idx = 0;
    char **results = malloc(capacity * sizeof(char *));
    if (!results) {
        udev_enumerate_unref(enumerate);
        udev_unref(udev);
        return NULL;
    }

    udev_list_entry_foreach(entry, devices) {
        const char *path = udev_list_entry_get_name(entry);
        struct udev_device *dev = udev_device_new_from_syspath(udev, path);

        const char *devnode = udev_device_get_devnode(dev);
        const char *prop = udev_device_get_property_value(dev, "ID_INPUT_KEYBOARD");
        if (prop && strcmp(prop, "1") == 0 && devnode) {
            if (idx >= capacity) {
                capacity *= 2;
                char **tmp = realloc(results, capacity * sizeof(char *));
                if (!tmp) {
                    for (size_t i = 0; i < idx; i++)
                        free(results[i]);
                    free(results);
                    udev_device_unref(dev);
                    udev_enumerate_unref(enumerate);
                    udev_unref(udev);
                    return NULL;
                }
                results = tmp;
            }
            results[idx++] = strdup(devnode);
        }
        udev_device_unref(dev);
    }

    udev_enumerate_unref(enumerate);
    udev_unref(udev);

    *count = idx;
    return results;
}

int open_keyboard(const char *path) {
    int fd = open(path, O_RDONLY | O_NONBLOCK);
    if (fd < 0)
        perror(path);
    return fd;
}

void kbd(void) {
    size_t count = 0;
    char **kbds = NULL;
    struct pollfd *pfds = NULL;
    int *fds = NULL;
    int ctrl = 0, alt = 0, j = 0;

    while (1) {
        // Refresh device list and fds if needed
        if (!kbds) {
            kbds = find_all_kbds(&count);
            if (!kbds || count == 0) {
                fprintf(stderr, "No keyboards found\n");
                free(kbds);
                kbds = NULL;
                sleep(1);
                continue;
            }

            pfds = calloc(count, sizeof(struct pollfd));
            fds = calloc(count, sizeof(int));
            if (!pfds || !fds) {
                perror("calloc");
                for (size_t i = 0; i < count; i++) free(kbds[i]);
                free(kbds);
                free(pfds);
                free(fds);
                return;
            }

            for (size_t i = 0; i < count; i++) {
                fds[i] = open_keyboard(kbds[i]);
                pfds[i].fd = fds[i];
                pfds[i].events = POLLIN;
            }
        }

        int ret = poll(pfds, count, -1);
        if (ret < 0) {
            if (errno == EINTR) continue;
            perror("poll");
            break;
        }

        for (size_t i = 0; i < count; i++) {
            if (pfds[i].fd < 0) continue;

            if (pfds[i].revents & POLLIN) {
                struct input_event ev;
                ssize_t n = read(fds[i], &ev, sizeof(ev));
                if (n == sizeof(ev)) {
                    if (ev.type == EV_KEY) {
                        if (ev.code == KEY_LEFTCTRL || ev.code == KEY_RIGHTCTRL)
                            ctrl = ev.value;
                        else if (ev.code == KEY_LEFTALT || ev.code == KEY_RIGHTALT)
                            alt = ev.value;
                        else if (ev.code == KEY_J)
                            j = ev.value;

                        if (ctrl && alt && j == 1)
                            ctl();
                    }
                } else if (n <= 0) {
                    // Treat this as device disconnect
                    close(fds[i]);
                    fds[i] = -1;
                    pfds[i].fd = -1;
                }
            }
            if (pfds[i].revents & (POLLHUP | POLLERR)) {
                close(fds[i]);
                fds[i] = -1;
                pfds[i].fd = -1;
            }
        }

        // If all fds are closed, free and reload devices on next iteration
        int all_closed = 1;
        for (size_t i = 0; i < count; i++) {
            if (pfds[i].fd >= 0) {
                all_closed = 0;
                break;
            }
        }
        if (all_closed) {
            for (size_t i = 0; i < count; i++) {
                free(kbds[i]);
            }
            free(kbds);
            free(fds);
            free(pfds);
            kbds = NULL;
            fds = NULL;
            pfds = NULL;
            ctrl = alt = j = 0;
            sleep(1);
        }
    }

    if (kbds) {
        for (size_t i = 0; i < count; i++) free(kbds[i]);
        free(kbds);
    }
    free(fds);
    free(pfds);
    return;
}

