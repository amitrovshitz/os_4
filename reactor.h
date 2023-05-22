#ifndef _REACTOR_H
#define _REACTOR_H

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>
#include <poll.h>

#define SERVER_PORT 9034
#define MAX_CLIENT  8192
#define BUFFER  1024

typedef void *(*handler_t)(int fd, void *react);
typedef struct pollfd pollfd_t, *pollfd_t_ptr;
typedef struct node_reactor node_reactor, *node_reactor_ptr;
typedef struct t_reactor t_reactor, *t_reactor_ptr;


struct node_reactor
{
	int fd;
    node_reactor_ptr next;
	union _hdlr_func_union
	{
		handler_t handler;
		void *handler_ptr;
	} hdlr;
};

struct t_reactor
{
	pthread_t thread;
	node_reactor_ptr head;
	pollfd_t_ptr fds;
	bool run;
};

void *createReactor();
void startReactor(void *this);
void stopReactor(void *this);
void addFd(void * this,int fd, handler_t handler);
void WaitFor(void *this);

void signal_handler();
void *client_function(int fd, void *this);
void *server_function(int fd, void *this);
#endif