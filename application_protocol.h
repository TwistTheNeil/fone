#include"shared_pipes.h"

/*
 * Sends a hello and returns a file descriptor to a named pipe
 * through which data is communicated through
 */
int send_hello() {
	int fd;
	char buf[PIPE_BUF];

	fd = open(fa2s_ctl, O_WRONLY);
	write(fd, "hello\n", 6);
	close(fd);

	sleep(1);

	fd = open(fs2a_ctl, O_RDONLY);
	read(fd, buf, PIPE_BUF);
	printf("Returned: %s\n", buf);
	close(fd);

	// TODO: create fd and send them back

}
