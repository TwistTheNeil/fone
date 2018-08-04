#include<string.h>
#include<stdio.h>
#include<pthread.h>
#include<stdlib.h>
#include<fcntl.h>
#include<sys/stat.h>
#include<unistd.h>
#include<limits.h>
#include"shared_pipes.h"
#include"fone_serial.c"

static void *process_client_r(void *arg);
static void *ctl_thread(void *arg);
static int create_pipe(const char *pipe);
static int create_pipes();

static int data_pipe_created = 0;

static void *process_client(void *arg) {
	char *name = (char *) arg;
	char *name_reader = calloc(40, sizeof(char));
	char *name_writer = calloc(40, sizeof(char));
	char *buf = calloc(PIPE_BUF, sizeof(char));
	int r_fd, w_fd;

	/* Create wronly pipe */
	name_writer[0] = 'f'; name_writer[1] = 's'; name_writer[2] = '2'; name_writer[3] = 'a';
	name_writer[4] = '_';
	strncat(name_writer, name, 32);
	// can we access though?
	if(create_pipe(name_writer) != 0) {
		fprintf(stderr, "[Fatal] Can't create data pipe (%s)\n", name_writer);
		return NULL; // TODO
	}

	/* Create rdonly pipe */
	name_reader[0] = 'f'; name_reader[1] = 'a'; name_reader[2] = '2'; name_reader[3] = 's';
	name_reader[4] = '_';
	strncat(name_reader, name, 32);
	// can we access though
	if(create_pipe(name_reader) != 0) {
		fprintf(stderr, "[Fatal] Can't create data pipe. Exiting ctl thread (%s)\n", name_reader);
		return NULL; // TODO
	}

	w_fd = open(name_writer, O_WRONLY);
	if(w_fd < 0) {
		fprintf(stderr, "[Fatal] Can't open pipe (%s) fd to process client write: error %d\n", name_writer, w_fd);
		return NULL; // TODO
	}
	r_fd = open(name_reader, O_RDONLY);
	if(r_fd < 0) {
		fprintf(stderr, "[Fatal] Can't open pipe (%s) fd to process client read: error %d\n", name, r_fd);
		return NULL; // TODO
	}

	while(1) {
		memset(buf, 0, PIPE_BUF);

		if(read(r_fd, buf, PIPE_BUF) == 0) {
			continue;
		}

		if(strncmp(buf, "finish", 6) != 0) {
			mq_push(buf, w_fd, r_fd);
		} else {
			mq.state = FINISHED;
			mq_cleanup();
			close(w_fd);
			close(r_fd);
 			unlink(name_reader);
 			unlink(name_writer);
			break;
		}
		sleep(1);
	}
}

/*
 * This is the control loop
 */
static void *ctl_thread(void *arg) {
	char *buf = calloc(PIPE_BUF, sizeof(char));
	char *random32 = calloc(32, sizeof(char));
	int fd, i;
	pthread_t client_processor;

	while(1) {
		memset(buf, PIPE_BUF, sizeof(char));
		memset(random32, 32, sizeof(char));

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

		if(pthread_create(&client_processor, NULL, process_client, (void*)random32)) {
			fprintf(stderr, "[Error] creating thread \"%s\"\n", random32);
			break; // TODO
		}

		mq.state = STARTED;

		fd = open(fs2a_ctl, O_WRONLY);
		if(fd < 0) {
			fprintf(stderr, "[Fatal] Can't open fs2a_ctl pipe fd: error %d\n", fd);
			break; // TODO
		}

		// TODO: What if client crashes and doesn't read?
		write(fd, random32, PIPE_BUF);
		close(fd);

		pthread_join(client_processor, NULL);
	}
}

static int create_pipe(const char *pipe) {
	if(access(pipe, F_OK) == -1) {
		if(mkfifo(pipe, 0666) != 0) {
			fprintf(stderr, "[Error] can't access or create pipe \"%s\"\n", pipe);
			return 1;
		}
	}
	return 0;
}

static int create_pipes() {
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
