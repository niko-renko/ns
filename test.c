#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <pthread.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <errno.h>

#define WATCH_DIR "/dev/input"
#define EVENT_BUF_LEN (1024 * (sizeof(struct inotify_event) + 16))

typedef struct event_file {
    char *name;
    struct event_file *next;
} event_file_t;

static event_file_t *event_list = NULL;
static pthread_mutex_t event_list_mutex = PTHREAD_MUTEX_INITIALIZER;

static void free_event_list() {
    event_file_t *cur = event_list;
    while (cur) {
        event_file_t *next = cur->next;
        free(cur->name);
        free(cur);
        cur = next;
    }
    event_list = NULL;
}

static void add_event_file(const char *name) {
    pthread_mutex_lock(&event_list_mutex);
    event_file_t *node = malloc(sizeof(event_file_t));
    node->name = strdup(name);
    node->next = event_list;
    event_list = node;
    pthread_mutex_unlock(&event_list_mutex);
}

static void remove_event_file(const char *name) {
    pthread_mutex_lock(&event_list_mutex);
    event_file_t **cur = &event_list;
    while (*cur) {
        if (strcmp((*cur)->name, name) == 0) {
            event_file_t *to_free = *cur;
            *cur = to_free->next;
            free(to_free->name);
            free(to_free);
            break;
        }
        cur = &(*cur)->next;
    }
    pthread_mutex_unlock(&event_list_mutex);
}

static void print_event_list() {
    pthread_mutex_lock(&event_list_mutex);
    event_file_t *cur = event_list;
    printf("Current event files:\n");
    while (cur) {
        printf("  %s\n", cur->name);
        cur = cur->next;
    }
    pthread_mutex_unlock(&event_list_mutex);
}

static void parse_existing_event_files() {
    DIR *dir = opendir(WATCH_DIR);
    if (!dir) return;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "event", 5) == 0) {
            add_event_file(entry->d_name);
        }
    }
    closedir(dir);
}

static void *watch_thread(void *arg) {
    int inotify_fd = inotify_init1(IN_NONBLOCK);
    if (inotify_fd < 0) return NULL;

    int wd = inotify_add_watch(inotify_fd, WATCH_DIR, IN_CREATE | IN_DELETE);
    if (wd < 0) {
        close(inotify_fd);
        return NULL;
    }

    char buffer[EVENT_BUF_LEN];
    while (1) {
        ssize_t length = read(inotify_fd, buffer, EVENT_BUF_LEN);
        if (length < 0) {
            if (errno == EAGAIN) {
                usleep(100000);
                continue;
            }
            break;
        }

        ssize_t i = 0;
        while (i < length) {
            struct inotify_event *event = (struct inotify_event *) &buffer[i];
            if (event->len && strncmp(event->name, "event", 5) == 0) {
                if (event->mask & IN_CREATE) {
                    add_event_file(event->name);
                    printf("Added: %s\n", event->name);
                } else if (event->mask & IN_DELETE) {
                    remove_event_file(event->name);
                    printf("Removed: %s\n", event->name);
                }
                print_event_list();
            }
            i += sizeof(struct inotify_event) + event->len;
        }
    }

    inotify_rm_watch(inotify_fd, wd);
    close(inotify_fd);
    return NULL;
}

int main() {
    parse_existing_event_files();
    print_event_list();

    pthread_t thread;
    pthread_create(&thread, NULL, watch_thread, NULL);


    while (1) {
    pthread_mutex_lock(&event_list_mutex);
    event_file_t *cur = event_list;
    while (cur) {
        printf("%s\n", cur->name);
        cur = cur->next;
    }
    pthread_mutex_unlock(&event_list_mutex);
    sleep(1);
    }
    

    pthread_join(thread, NULL);

    free_event_list();
    pthread_mutex_destroy(&event_list_mutex);
    return 0;
}
