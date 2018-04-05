#ifndef _router_h
#define _router_h

#include <time.h>
#include <sys/epoll.h>
#include <sys/socket.h>

// How often the routing server should update it's tables. In seconds.
#define DATA_UPDATE_INTERVAL 60
// How long a record should remain in the tables.
#define RECORD_REMAIN_TIME 180

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

/**
 * A struct used to store the route data. The next jump and cost to reach a target.
 */
struct route_data
{
    char target;
    char next_jump;
    char cost; // Also used to check whether a route is defined - Will always be 1 since all other nodes are 1 jump away.
    char learned_from;
    time_t last_seen;
};

#endif