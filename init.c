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

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>

#define DEV_PATH "/dev/input/event7"
#define SOCK_PATH "/run/initns.sock"

int file_add(const char *path, const char *s);

void die(const char *msg) {
	perror(msg);
	exit(1);
}

void vt_switch(int tty0, int tty63) {
	// Freeze all

	if (ioctl(tty0, VT_ACTIVATE, 63) < 0)
		die("VT_ACTIVATE");
	
	if (ioctl(tty0, VT_WAITACTIVE, 63) < 0)
		die("VT_WAITACTIVE");

	if (ioctl(tty63, KDSETMODE, KD_TEXT) < 0)
		die("KD_TEXT");

	if (ioctl(tty63, KDSKBMODE, K_UNICODE) < 0)
		die("KDSKBMODE");
}

void kbd(int tty0, int tty63) {
	int fd = open(DEV_PATH, O_RDONLY);
	if (fd < 0)
		die("kbd open");

	struct input_event ev;
	int ctrl = 0, alt = 0, j = 0;
	
	while (1) {
		ssize_t n = read(fd, &ev, sizeof(ev));
		if (n != sizeof(ev))
			die("kbd read");

		if (ev.type != EV_KEY)
			continue;

		if (ev.code == KEY_LEFTCTRL || ev.code == KEY_RIGHTCTRL)
			ctrl = ev.value;
		else if (ev.code == KEY_LEFTALT || ev.code == KEY_RIGHTALT)
			alt = ev.value;
		else if (ev.code == KEY_J)
			j = ev.value;

		if (ctrl && alt && j)
			vt_switch(tty0, tty63);
	}
}

void bash(int tty0, int tty63) {
	dup2(tty63, STDIN_FILENO);
	dup2(tty63, STDOUT_FILENO);
	dup2(tty63, STDERR_FILENO);

	vt_switch(tty0, tty63);
	execl("/bin/bash", "bash", (char *)NULL);
}

void cmd_new(int cfd, char *name) {
	file_add("/root/Code/instances", name);

	char rootfs[256];
	snprintf(rootfs, sizeof(rootfs), "/root/Code/%s", name);
	mkdir(rootfs, 0755);

	pid_t pid = fork();
	if (pid == 0)
		execl("/bin/tar", "tar", "xf", "/root/Code/image.tar", "--strip-components=1", "-C", rootfs, (char *) NULL);
	
	if (waitpid(pid, NULL, 0) == -1)
		die("waitpid");

	write(cfd, name, strlen(name));
}

void cmd(void) {
	int fd, cfd;
	struct sockaddr_un addr;
	char buf[256];
	ssize_t n;
	
	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd == -1)
		die("socket");
	
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, SOCK_PATH, sizeof(addr.sun_path) - 1);
	unlink(addr.sun_path);
	
	if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
		die("bind");
	
	if (listen(fd, 5) == -1)
		die("listen");

	for (;;) {
		cfd = accept(fd, NULL, NULL);

		if (cfd == -1) {
			if (errno == EINTR) continue;
			perror("accept");
			break;
		}
		
		while ((n = read(cfd, buf, sizeof(buf) - 1)) > 0) {
			buf[n] = '\0';
			char *nl = strchr(buf, '\n');
 			if (nl) *nl = '\0';
			char *cmd = strtok(buf, " ");
			char *arg = strtok(NULL, " ");

			if (!cmd)
				continue;

			if (strcmp(cmd, "new") == 0)
				cmd_new(cfd, arg);
		}
		
		close(cfd);
	}

	close(fd);
	unlink(SOCK_PATH);
}

int main(void) {
	setenv("PATH", "/bin:/usr/bin", 1);
	setenv("HOME", "/root", 1);
	cmd();
	for (;;) pause();


	if (setsid() < 0)
		die("setsid");

	int tty0 = open("/dev/tty0", O_RDWR);
	if (tty0 < 0)
		die("tty0 open");

	int tty63 = open("/dev/tty63", O_RDWR);
	if (tty63 < 0)
		die("tty63 open");

	pid_t pid;
	pid = fork();
	if (pid < 0)
		die("fork");
	if (pid == 0)
		bash(tty0, tty63);

	pid = fork();
	if (pid < 0)
		die("fork");
	if (pid == 0)
		kbd(tty0, tty63);

	pid = fork();
	if (pid < 0)
		die("fork");
	if (pid == 0)
		cmd();
		
	for (;;) pause();
}
