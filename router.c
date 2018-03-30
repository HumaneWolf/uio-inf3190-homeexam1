#include "ethernet.h"
#include "router.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/**
 * Add a file descriptor to the epoll.
 * Input:
 *      epctrl - Epoll controller struct.
 *      fd - The file descriptor to add.
 * Return:
 *      Returns 0 if successful.
 * Error:
 *      Will end the program in case of errors.
 */
int epoll_add(struct epoll_control *epctrl, int fd)
{
    struct epoll_event ev = {0};
    ev.events = EPOLLIN;
    ev.data.fd = fd;
    if (epoll_ctl(epctrl->epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1)
    {
        perror("epoll_add: epoll_ctl()");
        exit(EXIT_FAILURE);
    }
    return 0;
}

/**
 * Handle an incoming connection event from any socket.
 * Input:
 *      epctrl - The epoll controller struct.
 *      n - The event counter, says which event to handle.
 * Affected by:
 *      packetIsExpected, respBuffer, macCache, destinationMip, arpBuffer.
 */
void epoll_event(struct epoll_control *epctrl, int n)
{
        // Packet with routing data
    if (epctrl->events[n].data.fd == epctrl->routing_fd)
    {

    }
        // Packet for forwarding.
    else if (epctrl->events[n].data.fd == epctrl->forward_fd)
    {

    }
}

/**
 * Main method.
 * Affected by:
 *      packetIsExpected, myAddresses.
 */
int main(int argc, char *argv[])
{
    // Args count check
    if (argc <= 2)
    {
        printf("Syntax: %s <routing socket> <forwarding socket>\n", argv[0]);
        return EXIT_SUCCESS;
    }

    // Variables
    char *routing_sock_path = argv[1];
    char *forward_sock_path = argv[2];

    // Create the routing socket.
    int sock = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (sock == -1)
    {
        perror("main: socket()");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_un sockaddr;
    sockaddr.sun_family = AF_UNIX;
    strcpy(sockaddr.sun_path, routing_sock_path);

    // Unlink/delete old socket file before creating the new one.
    unlink(routing_sock_path);

    if (bind(sock, (struct sockaddr *)&sockaddr, sizeof(sockaddr)))
    {
        perror("main: bind()");
        exit(EXIT_FAILURE);
    }

    if (listen(sock, 100))
    {
        perror("main: listen()");
        exit(EXIT_FAILURE);
    }

    // Create epctrl control struct
    struct epoll_control epctrl;
    epctrl.routing_fd = sock;

    // Create the forwarding socket.
    sock = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (sock == -1)
    {
        perror("main: socket()");
        exit(EXIT_FAILURE);
    }

    sockaddr.sun_family = AF_UNIX;
    strcpy(sockaddr.sun_path, forward_sock_path);

    // Unlink/delete old socket file before creating the new one.
    unlink(forward_sock_path);

    if (bind(sock, (struct sockaddr *)&sockaddr, sizeof(sockaddr)))
    {
        perror("main: bind()");
        exit(EXIT_FAILURE);
    }

    if (listen(sock, 100))
    {
        perror("main: listen()");
        exit(EXIT_FAILURE);
    }

    // Create EPOLL.
    epctrl.forward_fd = sock;
    
    epctrl.epoll_fd = epoll_create(10);

    epoll_add(&epctrl, epctrl.routing_fd);
    epoll_add(&epctrl, epctrl.forward_fd);

    if (epctrl.epoll_fd == -1)
    {
        perror("main: epoll_create()");
        exit(EXIT_FAILURE);
    }

    printf("Ready to serve.\n");

    // Serve. The epoll_wait timeout makes this a "pulse" loop, meaning all periodic updates in the daemon
    // can be done from here.
    time_t last_update = 0;

    while (1)
    {
        int nfds, n;
        nfds = epoll_wait(epctrl.epoll_fd, epctrl.events, MAX_EVENTS, 20000); // Max waiting time = 20 sec.
        if (nfds == -1)
        {
            perror("main: epoll_wait()");
            exit(EXIT_FAILURE);
        }

        for (n = 0; n < nfds; n++)
        {
            // Handle event.
            epoll_event(&epctrl, n);
        }

        // If it is time to update the routing tables again.
        if (time(NULL) < (last_update - DATA_UPDATE_INTERVAL)) {

        }

        printf("Check done.\n");
    }

    // Close unix socket.
    close(epctrl.routing_fd);
    close(epctrl.forward_fd);

    return EXIT_SUCCESS;
}
