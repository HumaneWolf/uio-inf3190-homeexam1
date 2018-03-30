#ifndef _router_h
#define _router_h

#include <sys/epoll.h>
#include <sys/socket.h>

// How often the routing server should update it's tables. In seconds.
#define DATA_UPDATE_INTERVAL 60

#define MAX_EVENTS 20
/**
 * EPOLL control structure.
 */
struct epoll_control
{
    int epoll_fd;             // The file descriptor for the epoll.
    int routing_fd;           // The file descriptor for the router data exchange socket.
    int forward_fd;           // The file descriptor for the socket used to forward..
    struct epoll_event events[MAX_EVENTS];
};

#endif