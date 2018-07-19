#include<string.h>

typedef struct pipelist {
	char *name_in;
	char *name_out;
	int fd_in;
	int fd_out;
	struct pipelist *next;
} pipelist;

pipelist *head = NULL;
pipelist *tail = NULL;
int maxfd = -1;

/* 
 * These are the variables which hold the pipes required for sending
 * messages/commands between application clients and the server.
 * Any modifications will need to be reflected in create_pipes()
 */
const int N_PIPE_TYPES = 2;

static const char *statuspipe = "npipe_status";
static const char *sendsmspipe = "npipe_sendsmspipe";

/* Check if the pipes is already created. If not, then try to create them */
int create_pipes() {
	int i, j, err, fd = 0;
	const char **pipes = calloc(N_PIPE_TYPES, sizeof(char*));
	const char rw[2][3] = { "_r", "_w" };
	char *pipe;

	if(pipes == NULL) {
		fprintf(stderr, "[Error] create_pipes: Couldn't allocate memory\n");
		return 1;
	}

	pipes[0] = statuspipe;
	pipes[1] = sendsmspipe;

	for(i=0; i<N_PIPE_TYPES; i++) {
		for(j=0; j<2; j++) {
			pipe = calloc(strlen(pipes[i])+strlen(rw[j])+1, sizeof(char));
			if(pipe == NULL) {
				fprintf(stderr, "[Error] create_pipes: Couldn't allocate memory\n");
				free(pipes);
				return 1;
			}

			strncpy(pipe, pipes[i], strlen(pipes[i]));
			strncat(pipe, rw[j], strlen(rw[j]));
			pipe[strlen(pipes[i])+strlen(rw[j])] = 0;

			if(access(pipe, F_OK) == -1) {
				err = mkfifo(pipe, 0666);
				if(err != 0) {
					fprintf(stderr, "[Error] Can't access or create pipe (%s)\n", pipe);
					free(pipe);
					free(pipes);
					return 1;
				}
			}

			// open the pipe and record the fd
			if(strncmp(rw[j], "_r", 2) == 0) {
				fd = open(pipe, O_RDONLY | O_NONBLOCK);
			} else if (strncmp(rw[j], "_w", 2) == 0) {
				// Will probably be -1 since there is no thread reading from it
				// It will need to be dealt with when a read has come through
				fd = open(pipe, O_WRONLY | O_NONBLOCK);
			}

			if(fd > maxfd) {
				maxfd = fd;
			}

			// I will leave out the names for now
			// TODO: either add in the names or get rid of them from the struct
			if(j%2 == 0) {
				pipelist *temp = calloc(1, sizeof(pipelist));
				if(temp == NULL) {
					free(pipe);
					free(pipes);
					fprintf(stderr, "[Error] create_pipes: Couldn't allocate memory\n");
					return 1;
				}

				temp->fd_in = fd;

				if(head == NULL) {
					head = temp;
					tail = temp;
				} else {
					tail->next = temp;
					tail = temp;
				}
			} else {
				tail->fd_out = fd;
			}

			free(pipe);
		}
	}

	free(pipes);

	return 0;
}

void cleanup_pipes() {
	pipelist *i;

	while(head != NULL) {
		i = head;
		free(head->name_in);
		free(head->name_out);
		head = head->next;
		free(i);
	}
}
