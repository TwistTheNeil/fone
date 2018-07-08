#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<fcntl.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<unistd.h>
#include<limits.h>

static const char *fonepipe = "fonepipe";

int main() {
	int fd;
	char buf[PIPE_BUF];

int x = 0;

	while(x < 3) {
		fd = open(fonepipe, O_RDONLY);
		read(fd, buf, PIPE_BUF);
		printf("FREAD %s\n", buf);
		close(fd);

		fd = open(fonepipe, O_WRONLY);
		write(fd, "pw\n", 3);
		printf("FWRITE\n");
		close(fd);
		x++;
	}

	return 0;
}
			
