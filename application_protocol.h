#include<string.h>
#include"shared_pipes.h"

/*
 * Sends a hello and returns a file descriptor to a named pipe
 * through which data is communicated through
 */
int send_hello(int *in_fd, int *out_fd) {
	int fd;
	int retval;
	char buf[PIPE_BUF];
	char *pipename = calloc(40, sizeof(char));

	fd = open(fa2s_ctl, O_WRONLY);
	if(fd < 0) {
		fprintf(stderr, "[Error]: Couldn't communicate to ctl\n");
		return 1;
	}
	if(write(fd, "hello\n", 6) != 6) {
		fprintf(stderr, "[Error]: Couldn't send hello to ctl\n");
		return 1;
	}
	close(fd);

	sleep(1);

	fd = open(fs2a_ctl, O_RDONLY);
	if(fd < 0) {
		fprintf(stderr, "[Error]: Couldn't communicate to ctl\n");
		return 1;
	}
	if(read(fd, buf, PIPE_BUF) < 0) {
		fprintf(stderr, "[Error]: Couldn't read from ctl\n");
		return 1;
	}
	close(fd);


	pipename[0] = 'f'; pipename[1] = 's'; pipename[2] = '2'; pipename[3] = 'a';
	pipename[4] = '_';
	*in_fd = open(strncat(pipename, buf, 32), O_RDONLY);
	if(*in_fd < 0) {
		fprintf(stderr, "[Error]: Couldn't data pipe for reading (%s)\n", pipename);
		return 1;
	}

	pipename[1] = 'a'; pipename[3] = 's';
	*out_fd = open(pipename, O_WRONLY);
	if(*out_fd < 0) {
		fprintf(stderr, "[Error]: Couldn't data pipe for writing (%s)\n", pipename);
		return 1;
	}

	return 0;
}
