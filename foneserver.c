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
#include"pipelist.c"
#include"messagequeue.c"

/* Prototypes */
void graceful_exit();
void sigint_handler();
int uart_init();
void *com_read(void* arg);
void *com_write(void* arg):
void *write_serial(void* arg);
void *read_serial(void* arg);

typedef enum serverstate {
	RUNNING,
	TERMINATED
} serverstate;

/* Globals */
int fona_fd = -1;
int shared_com_state = 0; //change?
int shared_com_fd = -1;
pthread_t shared_com_reader;
pthread_t shared_com_writer;
const char *shared_com_pipe = "npipe_shared_com\0";
serverstate fonestate;

void graceful_exit() {
	cleanup_pipes();
	mq_cleanup();
}

void sigint_handler() {
	fprintf(stderr, "freeing memory\n");
	graceful_exit();
	exit(0);
}

int uart_init() {
	struct termios options;

	fona_fd = open("/dev/serial0", O_RDWR | O_NOCTTY | O_NDELAY);

	if(fona_fd < 0) {
		fprintf(stderr, "[Error] Can't open serial connection on /dev/serial0\n");
		return 1;
	}

	memset(&options, 0, sizeof(options));
	if(tcgetattr(fona_fd, &options) !=  0) {
		fprintf(stderr, "[Error] Getting tcgetattr\n");
		return 2;
	}

	cfsetospeed(&options, 115200);
	cfsetispeed(&options, 115200);

	options.c_cflag = (options.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
	options.c_iflag &= ~IGNBRK;         // disable break processing
	options.c_lflag = 0;                // no signaling chars, no echo,
	options.c_oflag = 0;                // no remapping, no delays
	options.c_cc[VMIN]  = 0;            // read doesn't block
	options.c_cc[VTIME] = 5;            // 0.5 seconds read timeout
	options.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl
	options.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
	options.c_cflag &= ~(PARENB | PARODD);      // no parity
	options.c_cflag |= 0;
	options.c_cflag &= ~CSTOPB;
	options.c_cflag &= ~CRTSCTS;

	if (tcsetattr(fona_fd, TCSANOW, &options) != 0) {
		fprintf(stderr, "[Error] Setting tcsetattr\n");
		return 3;
	}

	return 0;
}

int writter_to_shared = 0;

void *com_read(void* arg) {

}

void *com_write(void* arg) {
	
}

void *write_serial(void* arg) {
	while(1) {
		if(mq.head != NULL && mq.state == IDLE && (uart_init() == 0)) {
			mq.state = PROCESSING;

			if(pthread_create(&shared_com_reader, NULL, com_read, NULL) && pthread_create(&shared_com_writer, NULL, com_write, NULL)) {
				fprintf(stderr, "[Error] creating thread for shared_com\n");
				mq.state = IDLE;
			} else {
				write(fona_fd, (mq.head)->msg, strlen((mq.head)->msg));
			}

			//TODO: These don't belong here
			//pthread_join(shared_com_reader, NULL);
			//pthread_join(shared_com_writer, NULL);
		} else if(mq.com_head != NULL && mq.state == WORKING) {
			write(fona_fd, (mq.com_head)->msg, strlen((mq.com_head)->msg));
		}

		sleep(2);
	}
}

void *read_serial(void* arg) {
	char buf[PIPE_BUF];
	char pbuf[PIPE_BUF];
	int sequential_newline = 0;

	memset(buf, 0, PIPE_BUF);
	memset(pbuf, 0, PIPE_BUF);

	while(1) {
		if(mq.head != NULL && mq.state == PROCESSING) {
			//send client the pipe details

			//TODO: perhaps this else if should use mq.com_head?
		} else if(mq.head != NULL && mq.state == WORKING) {
			read(fona_fd, buf, PIPE_BUF);
			strncat(pbuf, buf, strlen(buf));
			if(strlen(pbuf) > 0 && pbuf[strlen(pbuf)-1] == '\n') {
				if(strlen(pbuf) > 1 && pbuf[strlen(pbuf)-2] == '\r') {
					write(mq.head->pipe_fd, pbuf, strlen(pbuf));
					mq_pop(CMD);
				//	sleep(1); // TODO: make sure this is not needed again - weird timing issue with main()
					memset(buf, 0, PIPE_BUF);
					memset(pbuf, 0, PIPE_BUF);
					mq.state = IDLE;
					close(fona_fd);
					continue;
				}
			}
		}

		// TODO: Sane sleep time? perhaps in nanoseconds?
		sleep(1);
	}
}

int main() {
	char buf[PIPE_BUF];
	fd_set rfds;
	int retval;
	pipelist *pipeindex;
	pthread_t read_serial_pthread, write_serial_pthread;

	signal(SIGINT, sigint_handler);

	if(create_pipes() != 0) {
		return 1;
	}

	/*if(uart_init() != 0) {
		return 1;
	}*/

	/*
	 * Create two threads:
	 *   read_serial  - read output from the serial interface and write
	 *                  it to the pipe for the clients to read
	 *   write_serial - read input from the pipelist head and write it to
	 *                  the serial interface
	 */
	if(pthread_create(&read_serial_pthread, NULL, read_serial, NULL)) {
		fprintf(stderr, "[Error] Failed to create a thread to read to serial\n");
		return 2;
	}

	if(pthread_create(&write_serial_pthread, NULL, write_serial, NULL)) {
		fprintf(stderr, "[Error] Failed to create a thread to write to serial\n");
		return 2;
	}

	fonestate = RUNNING;

	while(1) {
		// TODO: May need to check if IDLE
		FD_ZERO(&rfds);
		pipeindex = head;
		while(pipeindex != NULL) {
			FD_SET(pipeindex->fd, &rfds);
			pipeindex = pipeindex->next;
		}

		retval = select(maxfd+1, &rfds, NULL, NULL, NULL);
		if(mq.state == PROCESSING) {
			// prevent racing read with pipe
			continue;
		}
		if(retval == -1) {
			fprintf(stderr, "select() error\n");
		} else if(retval) {
			pipeindex = head;
			while(pipeindex != NULL) {
				if(FD_ISSET(pipeindex->fd, &rfds)) {
					break;
				}
				pipeindex = pipeindex->next;
			}

			memset(buf, 0, PIPE_BUF);
			read(pipeindex->fd, buf, PIPE_BUF);
			if(strncmp(buf, "AT", 2) == 0) {
				mq_push(buf, CMD, pipeindex->fd);
			}
			mq_print();
		}
		sleep(1);
	}

	/*
	 * Wait for threads to end
	 */
	if(pthread_join(read_serial_pthread, NULL)) {
		fprintf(stderr, "[Error] Joining read serial thread\n");
		return 3;
	}

	if(pthread_join(write_serial_pthread, NULL)) {
		fprintf(stderr, "[Error] Joining write serial thread\n");
		return 3;
	}

	graceful_exit();
	return 0;
}
