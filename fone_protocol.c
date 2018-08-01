#include<string.h>
#include<stdio.h>
#include<pthread.h>
#include<stdlib.h>
#include<fcntl.h>
#include<sys/stat.h>
//#include<sys/types.h>
#include<termios.h>
#include<unistd.h>
#include<limits.h>
#include<signal.h>
#include"messagequeue.c"

#include"shared_pipes.h"

void *ctl_thread(void *arg);
int create_pipe(const char *pipe);
int create_pipes();

/* 
 * These are the variables which hold the pipes required for sending
 * messages/commands between application clients and the server.
 * Any modifications will need to be reflected in create_pipes()
 */
void *ctl_thread(void *arg) {
	char *buf = calloc(PIPE_BUF, sizeof(char));
	char *random32 = calloc(40, sizeof(char));
	int fd, i;

	while(1) {
		memset(buf, PIPE_BUF, sizeof(char));
		memset(random32, 40, sizeof(char));

		fd = open(fa2s_ctl, O_RDONLY);
		if(fd < 0) {
			fprintf(stderr, "[Fatal] Can't open fa2s_ctl pipe fd: error %d\n", fd);
			break; // TODO
		}

		read(fd, buf, PIPE_BUF);
		close(fd);

		/* If not initiated properly, discard */
		if(strncmp(buf, "hello\n", 6) != 0) {
			continue;
		}


		/* Random name for a named pipe */
		srand(time(NULL));
		for(i=0; i<32; i++) {
			random32[5+i] = (char) (26 * (rand() / (RAND_MAX + 1.0))) + 97;
		}
		/* Create rdonly pipe */
		random32[0] = 'f'; random32[1] = 'a'; random32[2] = '2'; random32[3] = 's';
		random32[4] = '_';
		if(create_pipe(random32) != 0) {
			fprintf(stderr, "[Fatal] Can't create data pipe. Exiting ctl thread\n");
			break; // TODO
		}
		/* Create wronly pipe */
		random32[0] = 'f'; random32[1] = 's'; random32[2] = '2'; random32[3] = 'a';
		if(create_pipe(random32) != 0) {
			fprintf(stderr, "[Fatal] Can't create data pipe. Exiting ctl thread\n");
			break; // TODO
		}

		fd = open(fs2a_ctl, O_WRONLY);
		if(fd < 0) {
			fprintf(stderr, "[Fatal] Can't open fs2a_ctl pipe fd: error %d\n", fd);
			break; // TODO
		}

		// TODO: What if client crashes and doesn't read?
		write(fd, random32+5, PIPE_BUF);
		close(fd);
	}
}

int create_pipe(const char *pipe) {
	int err;

	if(access(pipe, F_OK) == -1) {
		err = mkfifo(pipe, 0666);
		if(err != 0) {
			fprintf(stderr, "[Error] can't access or create pipe \"%s\"\n", pipe);
			return 1;
		}
	}
	return 0;
}

int create_pipes() {
	pthread_t ctl_thread_t;

	if(create_pipe(fa2s_ctl) != 0) {
		return 1;
	}
	if(create_pipe(fs2a_ctl) != 0) {
		return 1;
	}

	if(pthread_create(&ctl_thread_t, NULL, ctl_thread, NULL)) {
		fprintf(stderr, "[Error] creating thread for fa2s_ctl\n");
		return 1;
	}

	return 0;	
}


int main() {
	create_pipes();
	while(1) {
		sleep(1);
	}
	return 0;
}
