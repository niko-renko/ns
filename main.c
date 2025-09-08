#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include "cgroup/cgroup.h"
#include "cmd/cmd.h"
#include "common.h"
#include "kbd/kbd.h"
#include "state/state.h"

int main(void) {
    State *state = init_state();
    set_state(state);

    char images[PATH_MAX];
    snprintf(images, PATH_MAX, "%s/images", ROOT);
    char rootfs[PATH_MAX];
    snprintf(rootfs, PATH_MAX, "%s/rootfs", ROOT);

    if (mkdir(ROOT, 0755) == -1)
        if (errno != EEXIST)
            die("ROOT mkdir");
    if (mkdir(images, 0755) == -1)
        if (errno != EEXIST)
            die("ROOT mkdir");
    if (mkdir(rootfs, 0755) == -1)
        if (errno != EEXIST)
            die("ROOT mkdir");

    char log[PATH_MAX];
    snprintf(log, PATH_MAX, "%s/log", ROOT);

    clean_fds();
    int fd = open(log, O_WRONLY | O_APPEND | O_CREAT, 0644);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);

    init_cgroup();
    spawn_kbd();
    spawn_sock_cmd();

    for (;;)
        pause();
}
