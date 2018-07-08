#include<stdio.h>
#include<pthread.h>
#include<stdlib.h>
#include<fcntl.h>
#include<sys/stat.h>
//#include<sys/types.h>
#include<unistd.h>
#include<limits.h>

static const char *fonepipe = "fonepipe";

void *read_serial_pthread(void* arg) {
	int fd;
	char buf[PIPE_BUF];

	while(1) {
		fd = open(fonepipe, O_WRONLY);
		write(fd, "cp\n", 3);
		printf("FWRITE\n");
		close(fd);
		sleep(2);
	}

}

void *write_serial_pthread(void* arg) {
	int fd;
	char buf[PIPE_BUF];

	while(1) {
		printf("started reading pipe\n");
		fd = open(fonepipe, O_RDONLY);
		read(fd, buf, PIPE_BUF);
		printf("FREAD %s\n", buf);
		close(fd);
		sleep(2);
	}

}

int main() {
	int err;
	pthread_t read_serial, write_serial;

	/*
	 * Check if the pipe is already created. If not, then try to create it
	 */
	if(access(fonepipe, F_OK) == -1) {
		err = mkfifo(fonepipe, 0666);
		if(err != 0) {
			fprintf(stderr, "[Error] Can't access pipe (%s)\n", fonepipe);
			return 1;
		}
	}

	/*
	 * Create two threads:
	 *   read_serial  - read output from the serial interface and write
	 *                  it to the pipe for the clients to read
	 *   write_serial - read input from the pipe and write it to the
	 *                  serial interface
	 */ 
	if(pthread_create(&read_serial, NULL, read_serial_pthread, NULL)) {
		fprintf(stderr, "[Error] Failed to create a thread to read to serial\n");
		return 2;
	}

	if(pthread_create(&write_serial, NULL, write_serial_pthread, NULL)) {
		fprintf(stderr, "[Error] Failed to create a thread to write to serial\n");
		return 2;
	}


	/*
	 * Wait for threads to end
	 */
	if(pthread_join(read_serial, NULL)) {
		fprintf(stderr, "[Error] Joining read serial thread\n");
		return 3;
	}

	if(pthread_join(write_serial, NULL)) {
		fprintf(stderr, "[Error] Joining write serial thread\n");
		return 3;
	}

	return 0;
}
