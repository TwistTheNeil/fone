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
	int fd, err;
	char buf[PIPE_BUF];

	if(access(fonepipe, F_OK) == -1) {
		err = mkfifo(fonepipe, 0666);
		if(err != 0) {
			fprintf(stderr, "[Error] Can't access pipe (%s)\n", fonepipe);
			return 1;
		}
	}

	while(1) {
		fd = open(fonepipe, O_WRONLY);
		write(fd, "cp\n", 3);
		printf("FWRITE\n");
		close(fd);

		fd = open(fonepipe, O_RDONLY);
		read(fd, buf, PIPE_BUF);
		printf("FREAD %s\n", buf);
		close(fd);
	}

	return 0;
}
			
