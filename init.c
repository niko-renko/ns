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

#define CGROUP_PATH "/sys/fs/cgroup"
#define DEV_PATH "/dev/input/event7"
#define SOCK_PATH "/run/initns.sock"

int file_add(const char *path, const char *s);
int file_contains(const char *path, const char *s);
int file_remove(const char *path, const char *s);

pid_t clone(int cfd) {
	struct clone_args args;
	memset(&args, 0, sizeof(args));
	args.flags = CLONE_INTO_CGROUP | CLONE_NEWPID | CLONE_NEWNS | CLONE_NEWCGROUP;
	args.exit_signal = SIGCHLD;
	args.cgroup = cfd;
	return syscall(SYS_clone3, &args, sizeof(args));
}

int rm_r(const char *path) {
    char cmd[512];
    int ret;

    if (snprintf(cmd, sizeof(cmd), "rm -rf '%s'", path) >= (int)sizeof(cmd))
        return -1;

    ret = system(cmd);
    return (ret == 0) ? 0 : -1;
}

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
	if (file_contains("/root/Code/instances", name)) {
		write(cfd, "exists\n", 7);
		return;
	}

	file_add("/root/Code/instances", name);

	char rootfs[256];
	snprintf(rootfs, sizeof(rootfs), "/root/Code/%s", name);
	mkdir(rootfs, 0755);

	pid_t pid = fork();
	if (pid == 0)
		execl("/bin/tar", "tar", "xf", "/root/Code/image.tar", "--strip-components=1", "-C", rootfs, (char *) NULL);
	
	if (waitpid(pid, NULL, 0) == -1)
		die("waitpid");

	write(cfd, "ok\n", 3);
}

void cmd_rm(int cfd, char *name) {
	if (!file_contains("/root/Code/instances", name)) {
		write(cfd, "notfound\n", 9);
		return;
	}

	char rootfs[256];
	snprintf(rootfs, sizeof(rootfs), "/root/Code/%s", name);
	rm_r(rootfs);

	file_remove("/root/Code/instances", name);

	write(cfd, "ok\n", 3);
}

void cmd_run(int cfd, char *name, int tty0) {
    	if (ioctl(tty0, VT_ACTIVATE, 1) < 0)
    		die("VT_ACTIVATE");

    	if (ioctl(tty0, VT_WAITACTIVE, 1) < 0)
		die("VT_WAITACTIVATE");

	char cgpath[4096];
    	snprintf(cgpath, sizeof(cgpath), "%s/%s", CGROUP_PATH, name);

	if (mount("cgroup", CGROUP_PATH, "cgroup2", 0, NULL) < 0 && errno != EBUSY)
		die("cgroup");
	
	if (mkdir(cgpath, 0755) == -1 && errno != EEXIST)
		die("mkdir");

	int fd = open(cgpath, O_DIRECTORY);
	if (fd < 0)
		die("cgroup open");

	pid_t pid = clone(fd);
	if (pid < 0)
		die("clone3");
	if (pid > 0) return;
	close(fd);

	if (mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL) < 0)
	    die("mount MS_PRIVATE failed");

	char rootfs[256];
	snprintf(rootfs, sizeof(rootfs), "/root/Code/%s", name);

	char rootfsmnt[256];
	snprintf(rootfsmnt, sizeof(rootfsmnt), "/root/Code/%s/mnt", name);

	if (mount(rootfs, rootfs, NULL, MS_BIND | MS_REC, NULL) < 0)
		die("bind /mnt/newroot");
	
	if (syscall(SYS_pivot_root, rootfs, rootfsmnt) < 0)
		die("pivot_root");

	if (umount2("/mnt", MNT_DETACH) < 0)
		die("umount2");

	execl("/sbin/init", "init", (char *)NULL);
}

void cmd(int tty0) {
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
			if (strcmp(cmd, "rm") == 0)
				cmd_rm(cfd, arg);
			if (strcmp(cmd, "run") == 0)
				cmd_run(cfd, arg, tty0);
		}
		
		close(cfd);
	}

	close(fd);
	unlink(SOCK_PATH);
}

int main(void) {
	setenv("PATH", "/bin:/usr/bin", 1);
	setenv("HOME", "/root", 1);

	if (setsid() < 0)
		die("setsid");

	int tty0 = open("/dev/tty0", O_RDWR);
	if (tty0 < 0)
		die("tty0 open");

	int tty63 = open("/dev/tty63", O_RDWR);
	if (tty63 < 0)
		die("tty63 open");

	cmd(tty0);
	for (;;) pause();

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
		cmd(tty0);
		
	for (;;) pause();
}
