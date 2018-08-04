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
#include"fone_protocol.c"

/* Prototypes */
void graceful_exit();
void sigint_handler();

void graceful_exit() {
	mq_cleanup();
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

	create_pipes();

	signal(SIGINT, sigint_handler);

	if(pthread_create(&read_serial_pthread, NULL, read_serial, NULL)) {
		fprintf(stderr, "[Error] Failed to create a thread to read to serial\n");
		return 2;
	}

	if(pthread_create(&write_serial_pthread, NULL, write_serial, NULL)) {
		fprintf(stderr, "[Error] Failed to create a thread to write to serial\n");
		return 2;
	}
	while(1) {
		sleep(1);
	}
	return 0;
}
