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

int fona_fd = -1;

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

void *write_serial(void* arg) {
	// TODO: do we need a buf?
	char buf[PIPE_BUF];

	while(1) {
		write(fona_fd, "AT\r\n", 4);
		printf("wrote\n");
		sleep(2);
	}

}

void *read_serial(void* arg) {
	char buf[PIPE_BUF];

	while(1) {
		read(fona_fd, buf, PIPE_BUF);
		// TODO: Sane sleep time?
		printf("read: %s\n", buf);
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

	if(uart_init() != 0) {
		return 1;
	}

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

	while(1) {
		FD_ZERO(&rfds);
		pipeindex = head;
		while(pipeindex != NULL) {
			FD_SET(pipeindex->fd, &rfds);
			pipeindex = pipeindex->next;
		}

		retval = select(maxfd+1, &rfds, NULL, NULL, NULL);

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

			read(pipeindex->fd, buf, PIPE_BUF);
			printf("read: %s\n", buf);
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
