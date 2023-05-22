#include "reactor.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

void *function_to_run(void *this) {
	if (this == NULL)
	{
		return NULL;
	}

	t_reactor_ptr reactor = (t_reactor_ptr)this;
    if(reactor == NULL)
    {
        return NULL;
    }
	while (reactor->run)
	{
		size_t size = 0;
		node_reactor_ptr temp = reactor->head;
		while(temp != NULL)
		{
			size++;
			temp = temp->next;
		}
		reactor->fds = (pollfd_t_ptr)calloc(size, sizeof(pollfd_t));
		if (reactor->fds == NULL)
		{
			return NULL;
		}
        temp = reactor->head;
        size_t i = 0;
		while(temp != NULL)
		{
			reactor->fds[i].fd = temp->fd;
			reactor->fds[i].events = POLLIN;
			temp = temp->next;
			i++;
		}
		int ret = poll(reactor->fds, i, -1);
		if (ret <= 0)
		{
			free(reactor->fds);
			reactor->fds = NULL;
			return NULL;
		}
		for (i = 0; i < size; ++i)
		{
			if ((reactor->fds[i]).revents & POLLIN)
			{
				node_reactor_ptr temp = reactor->head;
				for (unsigned int j = 0; j < i; ++j)
					temp = temp->next;
				void *handler_ret = temp->hdlr.handler((*(reactor->fds + i)).fd, reactor);

				if (handler_ret == NULL && (*(reactor->fds + i)).fd != reactor->head->fd)
				{
					node_reactor_ptr curr_node = reactor->head;
					node_reactor_ptr prev_node = NULL;
					while (curr_node != NULL && curr_node->fd != (*(reactor->fds + i)).fd)
					{
						prev_node = curr_node;
						curr_node = curr_node->next;
					}
					prev_node->next = curr_node->next;
					free(curr_node);
				}
				continue;
			}
			else if (((*(reactor->fds + i)).revents & POLLHUP || (*(reactor->fds + i)).revents & POLLNVAL || (*(reactor->fds + i)).revents & POLLERR) && (*(reactor->fds + i)).fd != reactor->head->fd)
			{
				node_reactor_ptr curr_node = reactor->head;
				node_reactor_ptr prev_node = NULL;

				while (curr_node != NULL && curr_node->fd != (*(reactor->fds + i)).fd)
				{
					prev_node = curr_node;
					curr_node = curr_node->next;
				}
				prev_node->next = curr_node->next;
				free(curr_node);
			}
		}
		free(reactor->fds);
		reactor->fds = NULL;
	}
	return reactor;
}

void *createReactor() {
	t_reactor_ptr reactor = NULL;

	if ((reactor = (t_reactor_ptr)malloc(sizeof(t_reactor))) == NULL)
	{
		return NULL;
	}

	reactor->thread = 0;
	reactor->head = NULL;
	reactor->fds = NULL;
	reactor->run = false;
	return reactor;
}

void startReactor(void *this) {
	if (this == NULL)
	{
		return;
	}

	t_reactor_ptr reactor = (t_reactor_ptr)this;

	if (reactor->head == NULL)
	{
		return;
	}

	else if (reactor->run)
	{
		return;
	}

	reactor->run = true;

	int return_value = pthread_create(&reactor->thread, NULL, function_to_run, this);

	if (return_value != 0)
	{
		reactor->run = false;
		reactor->thread = 0;
		return;
	}

}

void stopReactor(void *this) {
	if (this == NULL)
	{
		return;
	}
	t_reactor_ptr reactor = (t_reactor_ptr)this;
	void *thread_return = NULL;
	if (!reactor->run)
	{
		return;
	}
	reactor->run = false;
	int return_value = pthread_cancel(reactor->thread);
	if (return_value != 0)
	{
		return;
	}
	return_value = pthread_join(reactor->thread, &thread_return);
	if (return_value != 0 || thread_return == NULL)
	{
		return;
    }
	if (reactor->fds != NULL)
	{
		free(reactor->fds);
		reactor->fds = NULL;
	}
	reactor->thread = 0;
}

void addFd(void *this, int fd, handler_t handler) {
	if (this == NULL || handler == NULL || fd < 0 || fcntl(fd, F_GETFL) == -1 || errno == EBADF)
	{
		return;
	}
	t_reactor_ptr reactor = (t_reactor_ptr)this;
	node_reactor_ptr node = (node_reactor_ptr)malloc(sizeof(node_reactor));
    if (node == NULL)
	{
		return;
	}
	node->hdlr.handler = handler;
	node->next = NULL;
    node->fd = fd;
    if (reactor->head == NULL)
		reactor->head = node;
    else
	{
		node_reactor_ptr temp = reactor->head;
	    while (temp->next != NULL)
			temp = temp->next;
		temp->next = node;
	}
}

void WaitFor(void *this) {
	if (this == NULL)
	{
		return;
	}

	t_reactor_ptr reactor = (t_reactor_ptr)this;
	void *thread_return = NULL;

	if (!reactor->run)
		return;

    int return_value = pthread_join(reactor->thread, &thread_return);
	
	if (return_value != 0)
	{
		return;
	}
}