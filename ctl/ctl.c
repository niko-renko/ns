void clone_shell(void) {
    pid_t pid = fork();
    if (pid < 0)
        die("fork");
    if (pid > 0)
        return;
	clean_fds();

    int tty63 = open("/dev/tty63", O_RDWR | O_NOCTTY);
    if (tty63 < 0)
        die("tty63 open");
	dup2(tty63, STDOUT_FILENO);
	dup2(tty63, STDERR_FILENO);
    if (ioctl(tty63, TIOCSCTTY, (void *)1) < 0) 
        die("TIOCSCTTY");

	setenv("PATH", "/bin:/usr/bin", 1);
	setenv("HOME", "/root", 1);
	execl("/bin/bash", "bash", (char *)NULL);
}

static void clone_pkill(int sid) {
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

void start_ctl(void) {
	State *state = get_state();
	pthread_mutex_lock(&state->lock);
	if (state->ctl)
        clone_pkill(state->ctl);
	state->ctl = clone_shell();
	pthread_mutex_unlock(&state->lock);

	vt_text();
	vt_mode(VT_PROCESS);
	vt_switch(63);
}

void stop_ctl(void) {
	State *state = get_state();
	pthread_mutex_lock(&state->lock);
	if (state->ctl)
        clone_pkill(state->ctl);
	state->ctl = 0;
	pthread_mutex_unlock(&state->lock);

	vt_text();
	vt_mode(VT_AUTO);
	vt_switch(1);
}