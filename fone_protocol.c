#include"shared_pipes.h"
#include"fone_serial.c"

static int create_data_pipes(char *name_reader, char *name_writer, char *name);
static void *process_subscriber(void *arg);
static void *process_client(void *arg);
static void *ctl_thread(void *arg);
static int create_pipe(const char *pipe);

static int create_data_pipes(char *name_reader, char *name_writer, char *name) {
	/* Create wronly pipe */
	snprintf(name_writer, PIPE_NAME_LENGTH, "/tmp/fs2a_");
	strncat(name_writer, name, 32);
	// can we access though?
	if(create_pipe(name_writer) != 0) {
		fprintf(stderr, "[Fatal] Can't create data pipe (%s)\n", name_writer);
		return 1;
	}

	/* Create rdonly pipe */
	snprintf(name_reader, PIPE_NAME_LENGTH, "/tmp/fa2s_");
	strncat(name_reader, name, 32);
	// can we access though
	if(create_pipe(name_reader) != 0) {
		fprintf(stderr, "[Fatal] Can't create data pipe. Exiting ctl thread (%s)\n", name_reader);
		return 1;
	}

	return 0;
}

static void *process_subscriber(void *arg) {
	char *name = (char *) arg;
	char *name_reader = calloc(PIPE_NAME_LENGTH, sizeof(char));
	char *name_writer = calloc(PIPE_NAME_LENGTH, sizeof(char));
	char *buf = calloc(PIPE_BUF, sizeof(char));
	int r_fd, w_fd;

	if(create_data_pipes(name_reader, name_writer, name) != 0) {
		return NULL; //TODO
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

		if(strncmp(buf, "finish", 6) == 0) {
			subscription_remove_by_fd(w_fd);
			close(w_fd);
			close(r_fd);
 			unlink(name_reader);
 			unlink(name_writer);
			break;
		} else {
			sq_push(buf, w_fd, r_fd);
		}

		sleep(1);
	}
	return NULL;
}

static void *process_client(void *arg) {
	char *name = (char *) arg;
	char *name_reader = calloc(PIPE_NAME_LENGTH, sizeof(char));
	char *name_writer = calloc(PIPE_NAME_LENGTH, sizeof(char));
	char *buf = calloc(PIPE_BUF, sizeof(char));
	int r_fd, w_fd;

	if(create_data_pipes(name_reader, name_writer, name) != 0) {
		return NULL; //TODO
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
	return NULL;
}

/*
 * This is the control loop
 */
static void *ctl_thread(void *arg) {
	char *buf = calloc(PIPE_BUF, sizeof(char));
	char *random32 = calloc(32, sizeof(char));
	int fd, i;
	pthread_t client_processor, subscriber_processor;

	while(1) {
		memset(buf, PIPE_BUF, sizeof(char));

		fd = open(fa2s_ctl, O_RDONLY);
		if(fd < 0) {
			fprintf(stderr, "[Fatal] Can't open fa2s_ctl pipe fd: error %d\n", fd);
			break; // TODO
		}

		read(fd, buf, PIPE_BUF);
		close(fd);

		/* If not initiated properly, discard */
		if(strncmp(buf, "hello", 5) != 0) {
			if(strncmp(buf, "subscribe", 9) != 0) {
				continue;
			}
		}

		/* Random name for a named pipe */
		memset(random32, 32, sizeof(char));
		srand(time(NULL));
		for(i=0; i<32; i++) {
			random32[i] = (char) (26 * (rand() / (RAND_MAX + 1.0))) + 97;
		}

		if(strncmp(buf, "hello", 5) == 0) {
			if(pthread_create(&client_processor, NULL, process_client, (void*)random32)) {
				fprintf(stderr, "[Error] creating thread \"%s\"\n", random32);
				break; // TODO
			}

			mq.state = STARTED;
		} else if(strncmp(buf, "subscribe", 9) == 0) {
			if(pthread_create(&subscriber_processor, NULL, process_subscriber, (void*)random32)) {
				fprintf(stderr, "[Error] creating thread \"%s\"\n", random32);
				break; // TODO
			}
		}

		fd = open(fs2a_ctl, O_WRONLY);
		if(fd < 0) {
			fprintf(stderr, "[Fatal] Can't open fs2a_ctl pipe fd: error %d\n", fd);
			break; // TODO
		}

		// TODO: What if client crashes and doesn't read?
		write(fd, random32, PIPE_BUF);
		close(fd);

		/* We do not want to block a subscriber thread */
		if(strncmp(buf, "hello", 5) == 0) {
			pthread_join(client_processor, NULL);
		}
	}
	return NULL;
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

static int create_ctl_pipes() {
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
