#include<string.h>

typedef struct pipelist {
	char *name_in;
	int fd;
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
const int N_PIPE_TYPES = 3;

static const char *statuspipe = "npipe_status";
static const char *sendsmspipe = "npipe_sendsmspipe";
static const char *configpipe = "npipe_config";

/* Check if the pipes is already created. If not, then try to create them */
int create_pipes() {
	int i, j, err, fd = 0;
	const char **pipes = calloc(N_PIPE_TYPES, sizeof(char*));
	char *pipe;

	if(pipes == NULL) {
		fprintf(stderr, "[Error] create_pipes: Couldn't allocate memory\n");
		return 1;
	}

	pipes[0] = statuspipe;
	pipes[1] = sendsmspipe;
	pipes[2] = configpipe;

	for(i=0; i<N_PIPE_TYPES; i++) {
		pipe = calloc(strlen(pipes[i])+1, sizeof(char));
		if(pipe == NULL) {
			fprintf(stderr, "[Error] create_pipes: Couldn't allocate memory\n");
			free(pipes);
			return 1;
		}

		strncpy(pipe, pipes[i], strlen(pipes[i]));
		pipe[strlen(pipes[i])] = 0;

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
		fd = open(pipe, O_RDWR | O_NONBLOCK);

		if(fd > maxfd) {
			maxfd = fd;
		}

		// I will leave out the names for now
		// TODO: either add in the names or get rid of them from the struct
		pipelist *temp = calloc(1, sizeof(pipelist));
		if(temp == NULL) {
			free(pipe);
			free(pipes);
			fprintf(stderr, "[Error] create_pipes: Couldn't allocate memory\n");
			return 1;
		}

		temp->fd = fd;

		if(head == NULL) {
			head = temp;
			tail = temp;
		} else {
			tail->next = temp;
			tail = temp;
		}

		free(pipe);
	}

	free(pipes);

	return 0;
}

void cleanup_pipes() {
	pipelist *i;

	while(head != NULL) {
		i = head;
		free(head->name_in);
		head = head->next;
		free(i);
	}
}
