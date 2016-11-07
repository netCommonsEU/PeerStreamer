#ifndef IO_UDP_HEADER
#define IO_UDP_HEADER

struct io_udp_header {
  uint8_t portdiff;
  uint16_t size;
} __attribute__((packed));

#endif