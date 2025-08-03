#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main() {
    printf("start\n");

    pid_t pid = fork();
    if (pid < 0) return 1;
    if (pid > 0) return 0;

    int fd = open("/dev/tty9", O_RDWR);
    if (fd < 0) {
	    printf("open failed\n");
	    return 1;
    }

    for (int i = 0; i <= 2; i++) {
        if (dup2(fd, i) < 0) {
        	printf("dup failed");
        	return 1;
        }
    }
    if (fd > 2) close(fd);

    if (setsid() < 0) {
            printf("setsid failed\n");
            return 1;
    }

    execl("/bin/bash", "bash", (char *)NULL);
    return 0;
}

