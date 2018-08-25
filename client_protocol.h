#include<string.h>
#include<signal.h>
#include"shared_pipes.h"

void init_fone_client() {
	signal(SIGPIPE, SIG_IGN);
}

/*
 * Sends a subscribe and returns a file descriptor to a named pipe
 * through which data is communicated through
 */
int send_subscribe(int *in_fd, int *out_fd) {
	int fd;
	char buf[PIPE_BUF];
	char *pipename = calloc(PIPE_NAME_LENGTH, sizeof(char));

	/*
	 * Open ctl pipe to send subscribe and receive
	 * a data pipe for further communication
	 */
	fd = open(fa2s_ctl, O_WRONLY);
	if(fd < 0) {
		fprintf(stderr, "[Error]: Couldn't communicate to ctl\n");
		return 1;
	}
	if(write(fd, "subscribe\n", 10) != 10) {
		fprintf(stderr, "[Error]: Couldn't send hello to ctl\n");
		return 1;
	}
	close(fd);

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

	/* Wait for server to create pipes */
	sleep(2);

	/*
	 * Construct data pipe names and
	 * open them for communication
	 */
	snprintf(pipename, PIPE_NAME_LENGTH, "/tmp/fs2a_");
	*in_fd = open(strncat(pipename, buf, 32), O_RDONLY);
	if(*in_fd < 0) {
		fprintf(stderr, "[Error]: Couldn't open data pipe for reading (%s)\n", pipename);
		return 1;
	}

	snprintf(pipename, PIPE_NAME_LENGTH, "/tmp/fa2s_");
	*out_fd = open(strncat(pipename, buf, 32), O_WRONLY);
	if(*out_fd < 0) {
		fprintf(stderr, "[Error]: Couldn't data pipe for writing (%s)\n", pipename);
		return 1;
	}

	free(pipename);
	return 0;
}

/*
 * Sends a hello and returns a file descriptor to a named pipe
 * through which data is communicated through
 */
int send_hello(int *in_fd, int *out_fd) {
	int fd;
	char buf[PIPE_BUF];
	char *pipename = calloc(PIPE_NAME_LENGTH, sizeof(char));

	/*
	 * Open ctl pipe to send hello and receive
	 * a data pipe for further communication
	 */
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

	/* Wait for server to create pipes */
	sleep(2);

	/*
	 * Construct data pipe names and
	 * open them for communication
	 */
	snprintf(pipename, PIPE_NAME_LENGTH, "/tmp/fs2a_");
	*in_fd = open(strncat(pipename, buf, 32), O_RDONLY);
	if(*in_fd < 0) {
		fprintf(stderr, "[Error]: Couldn't open data pipe for reading (%s)\n", pipename);
		return 1;
	}

	snprintf(pipename, PIPE_NAME_LENGTH, "/tmp/fa2s_");
	*out_fd = open(strncat(pipename, buf, 32), O_WRONLY);
	if(*out_fd < 0) {
		fprintf(stderr, "[Error]: Couldn't data pipe for writing (%s)\n", pipename);
		return 1;
	}

	free(pipename);
	return 0;
}

int send_finish(int *in_fd, int *out_fd) {
	if(write(*out_fd, "finish\n", 7) != 7) {
		fprintf(stderr, "[Error]: Couldn't send finish to ctl\n");
		close(*in_fd);
		close(*out_fd);
		return 1;
		// TODO: How would we handle this on the server side?
	}

	close(*in_fd);
	close(*out_fd);
	return 0;
}
