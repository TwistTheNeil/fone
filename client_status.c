#include<stdio.h>
#include<fcntl.h>
#include<unistd.h>
#include<limits.h>

static const char *statuspipe = "npipe_status";

int main() {
	int fd;
	char buf[PIPE_BUF];

	fd = open(statuspipe, O_WRONLY);
	write(fd, "AT\r\n", 4);
	close(fd);

	sleep(1);

	fd = open(statuspipe, O_RDONLY);
	read(fd, buf, PIPE_BUF);
	printf("Returned: %s\n", buf);
	close(fd);

	return 0;
}
