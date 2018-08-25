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
	char *buf;

	signal(SIGINT, sigint_handler);
	signal(SIGPIPE, SIG_IGN);

	mq_init();
	sq_init();

	if(uart_init() != 0) {
		fprintf(stderr, "[Fatal] Cannot open /dev/serial0 for r/w\n");
		return 1;
	}

	/* Disable Echo  */
	buf = strndup("ATE 0\r\n", 7);
	mq_push(buf, -1, -1);
	free(buf);

	/* Enable Caller ID  */
	buf = strndup("AT+CLIP=1\r\n", 11);
	mq_push(buf, -1, -1);
	free(buf);

	/* Set volume = 20  */
	buf = strndup("AT+CLVL=20\r\n", 12);
	mq_push(buf, -1, -1);
	free(buf);

	if(pthread_create(&read_serial_pthread, NULL, read_serial, NULL)) {
		fprintf(stderr, "[Error] Failed to create a thread to read to serial\n");
		return 2;
	}

	if(pthread_create(&write_serial_pthread, NULL, write_serial, NULL)) {
		fprintf(stderr, "[Error] Failed to create a thread to write to serial\n");
		return 2;
	}

	/* Wait until configured and purge config from queue */
	sleep(10);
	mq.state = FINISHED;
	mq_cleanup();

	create_ctl_pipes();
	printf("[Info] Foneserver is running.\n");

	pthread_join(write_serial_pthread, NULL);
	pthread_join(read_serial_pthread, NULL);
	uart_close();
	return 0;
}
