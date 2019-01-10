#include<stdio.h>
#include<pthread.h>
#include<stdlib.h>
#include<fcntl.h>
#include<sys/stat.h>
#include<termios.h>
#include<unistd.h>
#include<limits.h>
#include<signal.h>
#include"fone_protocol.c"

/* Prototypes */
void graceful_exit();
void sigint_handler();

void graceful_exit() {
	mq_cleanup();
	sq_cleanup();
	unlink(fa2s_ctl);
	unlink(fs2a_ctl);
}

void sigint_handler() {
	fprintf(stderr, "freeing memory\n");
	graceful_exit();
	exit(0);
}

int main() {
	pthread_t read_serial_pthread, write_serial_pthread;

	signal(SIGINT, sigint_handler);
	signal(SIGPIPE, SIG_IGN);

	mq_init();
	sq_init();

	if(uart_init() != 0) {
		fprintf(stderr, "[Fatal] Cannot open /dev/serial0 for r/w\n");
		return 1;
	}

	if(pthread_create(&read_serial_pthread, NULL, read_serial, NULL)) {
		fprintf(stderr, "[Error] Failed to create a thread to read to serial\n");
		return 2;
	}

	if(pthread_create(&write_serial_pthread, NULL, write_serial, NULL)) {
		fprintf(stderr, "[Error] Failed to create a thread to write to serial\n");
		return 2;
	}

	mq_cleanup();

	create_ctl_pipes();
	printf("[Info] Foneserver is running.\n");

	pthread_join(write_serial_pthread, NULL);
	pthread_join(read_serial_pthread, NULL);
	uart_close();
	return 0;
}
