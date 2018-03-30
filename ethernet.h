#ifndef _ethernet_h
#define _ethernet_h

#include <stdint.h>

/**
 * Max size for a single MIP packet, including header and payload.
 */
#define MAX_PACKET_SIZE 1500

/**
 * Max size for the content/payload.
 * Set lower than MAX_FRAME_SIZE to make sure there is space for the header.
 */
#define MAX_PAYLOAD_SIZE 1496

struct ethernet_frame
{
    uint8_t destination[6];
    uint8_t source[6];
    uint16_t protocol;
    char msg[];
} __attribute__((packed));

#endif