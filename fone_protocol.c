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

void *process_client_r(void *arg);
void *process_client_w(void *arg);
void *ctl_thread(void *arg);
int create_pipe(const char *pipe);
int create_pipes();

void *process_client_r(void *arg) {
	char *name = (char *) arg;
	char *buf = calloc(PIPE_BUF, sizeof(char));
	int fd;

	while(1) {
printf("i am in the loop\n");
		fd = open(name, O_RDONLY);
		if(fd < 0) {
			fprintf(stderr, "[Fatal] Can't open pipe (%s) fd to process client read: error %d\n", name, fd);
			//break; // TODO
			continue;
		}

		int x = read(fd, buf, PIPE_BUF); //do not declare here
printf("got from process reader[%d] %s\n", x, buf);
		close(fd);
		sleep(1);
	}
}

void *process_client_w(void *arg) {
	char *name = (char *) arg;
	char *buf = calloc(PIPE_BUF, sizeof(char));
	int fd;

	while(1) {
		printf("i am in the other loop\n");
		fd = open(name, O_WRONLY);
		if(fd < 0) {
			fprintf(stderr, "[Fatal] Can't open pipe (%s) fd to process client write: error %d\n", name, fd);
			break; // TODO
		}

		//write(fd, buf, PIPE_BUF);
printf("write something\n");
		close(fd);
		sleep(1);
	}
}

/*
 * This is the control loop
 */
void *ctl_thread(void *arg) {
	char *buf = calloc(PIPE_BUF, sizeof(char));
	char *random32 = calloc(32, sizeof(char));
	char *name_reader = calloc(40, sizeof(char));
	char *name_writer = calloc(40, sizeof(char));
	int fd, i;
	pthread_t client_read_processor, client_write_processor;

	while(1) {
printf("looping\n");
		memset(buf, PIPE_BUF, sizeof(char));
		memset(random32, 32, sizeof(char));
		memset(name_reader, 40, sizeof(char));
		memset(name_writer, 40, sizeof(char));

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
			random32[i] = (char) (26 * (rand() / (RAND_MAX + 1.0))) + 97;
		}

		/* Create rdonly pipe and start reading in a thread */
		name_reader[0] = 'f'; name_reader[1] = 'a'; name_reader[2] = '2'; name_reader[3] = 's';
		name_reader[4] = '_';
		strncat(name_reader, random32, 32);
		if(create_pipe(name_reader) != 0) {
			fprintf(stderr, "[Fatal] Can't create data pipe. Exiting ctl thread\n");
			break; // TODO
		}
		if(pthread_create(&client_read_processor, NULL, process_client_r, (void*)name_reader)) {
			fprintf(stderr, "[Error] creating thread \"%s\"\n", name_reader);
			break; // TODO
		}

		/* Create wronly pipe */
		name_writer[0] = 'f'; name_writer[1] = 's'; name_writer[2] = '2'; name_writer[3] = 'a';
		name_writer[4] = '_';
		strncat(name_writer, random32, 32);
		if(create_pipe(name_writer) != 0) {
			fprintf(stderr, "[Fatal] Can't create data pipe. Exiting ctl thread\n");
			break; // TODO
		}
		if(pthread_create(&client_write_processor, NULL, process_client_w, (void*)name_writer)) {
			fprintf(stderr, "[Error] creating thread \"%s\"\n", name_writer);
			break; // TODO	
		}

		fd = open(fs2a_ctl, O_WRONLY);
		if(fd < 0) {
			fprintf(stderr, "[Fatal] Can't open fs2a_ctl pipe fd: error %d\n", fd);
			break; // TODO
		}

		// TODO: What if client crashes and doesn't read?
		write(fd, random32, PIPE_BUF);
		close(fd);

		pthread_join(client_read_processor, NULL);
		pthread_join(client_write_processor, NULL);
printf("unlink pipes\n");
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
