
void vt_switch(int vt) {
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

void vt_text() {
    int tty63 = open("/dev/tty63", O_RDWR | O_NOCTTY);
    if (tty63 < 0)
        die("TIOCSCTTY");
	if (ioctl(tty63, KDSETMODE, KD_TEXT) < 0)
		die("KD_TEXT");
	if (ioctl(tty63, KDSKBMODE, K_UNICODE) < 0)
		die("KDSKBMODE");
    close(tty63);
}

void vt_mode(int modeval) {
    int tty63 = open("/dev/tty63", O_RDWR | O_NOCTTY);
    if (tty63 < 0)
        die("tty63 open");
    struct vt_mode mode = {0};
    mode.mode = modeval;
    mode.relsig = SIGWINCH;
    mode.acqsig = SIGWINCH;
    if (ioctl(tty63, VT_SETMODE, &mode) < 0)
        die("VT_SETMODE");
    close(tty63);
}