#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "common.h"

int main(int argc, char *argv[]) {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0)
        die("socket");

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCK_PATH, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        die("connect");

    for (int i = 1; i < argc; i++) {
        write(fd, argv[i], strlen(argv[i]));
        write(fd, " ", 1);
    }
    write(fd, "\n", 1);

    char buf[256];
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        for (ssize_t i = 0; i < n; i++) {
            write(STDOUT_FILENO, &buf[i], 1);
            if (buf[i] == '\n') {
                close(fd);
                return 0;
            }
        }
    }

    close(fd);
    return 0;
}