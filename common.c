#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include <linux/input.h>
#include <linux/vt.h>
#include <linux/kd.h>
#include <linux/sched.h>

#include <sys/mount.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>

void die(const char *msg) {
	perror(msg);
	//exit(1);
}

void clean_fds() {
	for (int fd = 0; fd < sysconf(_SC_OPEN_MAX); fd++)
        close(fd);
}

void clone_pkill(int sid) {
	pid_t pid = fork();
    if (pid < 0)
        die("fork");
	if (pid > 0)
		if (waitpid(pid, NULL, 0) == -1)
			die("waitpid");
		else
			return;
	clean_fds();
    char ssid[12];
    sprintf(ssid, "%d", sid);
	execl("/bin/pkill", "pkill", "-9", "-s", ssid, (char *) NULL);
}

void switch_vt(int vt) {
	int tty0 = open("/dev/tty0", O_RDWR);
	if (tty0 < 0)
		die("tty0 open");
    if (ioctl(tty0, VT_ACTIVATE, vt) < 0)
    	die("VT_ACTIVATE");
    while (ioctl(tty0, VT_WAITACTIVE, vt) < 0) {
        if (errno == EINTR) continue;
        perror("VT_WAITACTIVE");
        break;
    }
	close(tty0);
}
