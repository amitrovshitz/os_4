#include "reactor.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

void* reactor = NULL;
void signal_handler() {
	
	if (!reactor)
	{
		if (((t_reactor_ptr)reactor)->run)
			stopReactor(reactor);

		node_reactor_ptr temp = ((t_reactor_ptr)reactor)->head;
		node_reactor_ptr prev = NULL;

		while (temp)
		{
			prev = temp;
			temp = temp->next;
			close(prev->fd);
			free(prev);
		}
		free(reactor);
	}
	else
	    exit(EXIT_SUCCESS);
}

void *client_function(int fd, void *this) {
	char *buffer = (char *)calloc(BUFFER, sizeof(char));

	if (buffer == NULL)
	{
		close(fd);
		return NULL;
	}

	int read = recv(fd, buffer, BUFFER, 0);

	if (read <= 0)
	{
		free(buffer);
		close(fd);
		return NULL;
	}

    printf("%s",buffer);
	free(buffer);
	return this;
}

void *server_function(int socket, void *this) {
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);

	if ((t_reactor_ptr)this == NULL)
	{
		return NULL;
	}
	int client_fd = accept(socket, (struct sockaddr *)&client_addr, &client_len);
	if (client_fd < 0)
	{
		return NULL;
	}
	addFd(this, client_fd, client_function);
	return this;
}

int main(void) {
	struct sockaddr_in server_addr;
	int server_fd = -1, reuse = 1;

	signal(SIGINT, signal_handler);

	memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		return -1;
	}

	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) < 0)
	{
		close(server_fd);
		return -1;
	}

	if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		close(server_fd);
		return -1;
	}

	if (listen(server_fd, MAX_CLIENT) < 0)
	{
		close(server_fd);
		return -1;
	}
	reactor = createReactor();
	if (reactor == NULL)
	{
		close(server_fd);
		return -1;
	}

	addFd(reactor, server_fd, server_function);
	startReactor(reactor);
	WaitFor(reactor);
    signal_handler();
	return 0;
}