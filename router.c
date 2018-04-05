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
#include <unistd.h>


struct route_data targets[255];


/**
 * Add a file descriptor to the epoll.
 * Input:
 *      epctrl - Epoll controller struct.
 *      fd - The file descriptor to add.
 */
void epoll_add(struct epoll_control *epctrl, int fd)
{
    struct epoll_event ev = {0};
    ev.events = EPOLLIN;
    ev.data.fd = fd;
    if (epoll_ctl(epctrl->epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1)
    {
        perror("epoll_add: epoll_ctl()");
        exit(EXIT_FAILURE);
    }
}

/**
 * Handle an incoming connection event from any socket.
 * Input:
 *      epctrl - The epoll controller struct.
 *      n - The event counter, says which event to handle.
 * Global Vars:
 *      targets - Can add new potential targets to the list, as well as 
 */
void epoll_event(struct epoll_control *epctrl, int n)
{
        // Received routing data
    if (epctrl->events[n].data.fd == epctrl->routing_fd)
    {
        unsigned char source = 0;
        unsigned char length_buffer = 0;
        unsigned char *buffer;

        int ret = recv(epctrl->events[n].data.fd, &source, 1, 0);
        if (ret == -1) {
            perror("epoll_event: recv(routing 1)");
            return;
        }

        // Store source
        targets[source].cost = 1;
        targets[source].last_seen = time(NULL);
        targets[source].learned_from = source;
        targets[source].next_jump = source;
        targets[source].target = source;

        ret = recv(epctrl->events[n].data.fd, &length_buffer, 1, 0);
        if (ret == -1) {
            perror("epoll_event: recv(routing 2)");
            return;
        }

        // If no other routes are sent.
        if (length_buffer == 0) {
            // Get the rest of the buffer, since some blank bytes get sent.
            unsigned char buffer[3]; // 4 bytes of blank data sent, 1 is the counter, 3 left.
            ret = recv(epctrl->events[n].data.fd, &buffer, 3, 0);
            if (ret == -1) {
                perror("epoll_event: recv(routing 3)");
                return;
            }
            return;
        }

        buffer = malloc(length_buffer * 2); // * 2 because length buffer is number of routes.

        // If not divisible by 4, padding will be sent. Find the padding length.
        int length = 2 * length_buffer;
        for (; (length + 1) % 4 != 0; length++) {}
        // +1 to account for the number of addresses received, since that's also a part of the payload.

        ret = recv(epctrl->events[n].data.fd, buffer, length, 0);
        if (ret == -1) {
            perror("epoll_event: recv(routing 3)");
            return;
        }


        // Store data.
        for (int i = 0; i < length_buffer; i++)
        {
            if (targets[buffer[i * 2]].last_seen == 0 // It is unknown
                || (buffer[(i * 2) + 1] + 1) <= targets[buffer[i * 2]].cost) // Or the new route is "cheaper"/the same
            {
                targets[buffer[i * 2]].cost = buffer[(i * 2) + 1] + 1;
                targets[buffer[i * 2]].last_seen = time(NULL);
                targets[buffer[i * 2]].learned_from = source;
                targets[buffer[i * 2]].next_jump = source;
                targets[buffer[i * 2]].target = buffer[i * 2];
            }
        }

        free(buffer);

        printf("Handled incoming routing data from %u.\n", source);
    }
        // Forwarding
    else if (epctrl->events[n].data.fd == epctrl->forward_fd)
    {
        unsigned char buffer = 0;
        int ret = recv(epctrl->events[n].data.fd, &buffer, 1, 0);
        if (ret == -1) {
            perror("epoll_event: recv(forward)");
            return;
        }

        printf("Incoming forward of packet from %u\n", buffer);

        if (targets[(int)buffer].cost) { // If cost is at not 0, aka we know where it is.
            buffer = targets[(int)buffer].next_jump;
        } else {
            buffer = 255;
        }

        ret = send(epctrl->events[n].data.fd, &buffer, 1, 0);
        if (ret == -1) {
            perror("epoll_event: send(forward)");
            return;
        }

        printf("Handled forwarding of packet to %u.\n", buffer);
    }
}

/**
 * Main method.
 * Global vars:
 *      targets - Main can read the targets global variable, when sending route data information.
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
    if (sock == -1) {
        perror("main: socket()");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_un sockaddr;
    sockaddr.sun_family = AF_UNIX;
    strcpy(sockaddr.sun_path, routing_sock_path);

    if (connect(sock, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) == -1) {
        perror("main: connect(r)");
        exit(EXIT_FAILURE);
    }

    // Create epctrl control struct
    struct epoll_control epctrl;
    epctrl.routing_fd = sock;

    // Create the forwarding socket.
    sock = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (sock == -1) {
        perror("main: socket()");
        exit(EXIT_FAILURE);
    }

    sockaddr.sun_family = AF_UNIX;
    strcpy(sockaddr.sun_path, forward_sock_path);

    if (connect(sock, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) == -1) {
        perror("main: connect(f)");
        exit(EXIT_FAILURE);
    }

    // Create EPOLL.
    epctrl.forward_fd = sock;
    
    epctrl.epoll_fd = epoll_create(10);

    epoll_add(&epctrl, epctrl.routing_fd);
    epoll_add(&epctrl, epctrl.forward_fd);

    if (epctrl.epoll_fd == -1) {
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
        nfds = epoll_wait(epctrl.epoll_fd, epctrl.events, MAX_EVENTS, 1000); // Max waiting time = 1 sec.
        if (nfds == -1) {
            perror("main: epoll_wait()");
            exit(EXIT_FAILURE);
        }

        for (n = 0; n < nfds; n++)
        {
            // Handle event.
            epoll_event(&epctrl, n);
        }

        // If it is time to update the routing tables again.
        if (time(NULL) > (last_update + DATA_UPDATE_INTERVAL))
        {
            // Update timer
            last_update = time(NULL);

            // Check if it is about time to delete a route.
            for (int i = 0; i < 255; i++)
            {
                if (time(NULL) > (targets[i].last_seen + RECORD_REMAIN_TIME)) {
                    targets[i].cost = 0;
                    targets[i].last_seen = 0;
                    targets[i].learned_from = 0;
                    targets[i].next_jump = 0;
                    targets[i].target = 0;
                }
            }

            for (int i = 0; i < 255; i++) {
                // If the route doesn't exist as far as we know, don't send. Leave it for a broadcast packet later.
                if (targets[i].cost == 0) {
                    continue;
                }

                // Prepare sending updated routing data.
                unsigned char buffer[(255 * 2) + 2] = { 0 };
                unsigned char counter = 0;

                buffer[0] = i; // Send to that address.
                // Do not broadcast so we do not send routes to the node we learned of them from.
                // Split horizon implementation part A.

                // Build the list we're sending
                for (int j = 0; j < 255; j++)
                {
                    // If we did not learn about it from this interface AND it exists.
                    // Split horizon implementation part B.
                    if (targets[j].cost && targets[j].learned_from != i) {
                        buffer[(j * 2) + 2] = targets[j].next_jump;
                        buffer[(j * 2) + 3] = targets[j].cost;
                        counter++;
                    }
                }

                // If we know about no routes, don't send.
                if (counter == 0) {
                    continue;
                }

                buffer[1] = counter;

                // If the data is not disible by 4, send some blank data.
                int length = 1 + (2 * counter);
                for (; length % 4 != 0; length++) {}

                // + an additional 1 to give space for the destination address, not part of payload..
                int ret = send(epctrl.routing_fd, &buffer, 1 + length, 0);
                if (ret == -1) {
                    perror("main: send()");
                }
            }
        }

        unsigned char buffer[5] = { 0 };
        buffer[0] = 255;
        // Send 4 empty bytes + address.
        int ret = send(epctrl.routing_fd, &buffer, 5, 0);
        if (ret == -1) {
            perror("main: send(broadcast)");
        }

        printf("Check done.\n");
    }

    // Close unix socket.
    close(epctrl.routing_fd);
    close(epctrl.forward_fd);

    return EXIT_SUCCESS;
}
