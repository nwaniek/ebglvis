#ifndef __DAVIS_H__0EDF3A92_5F95_42CA_A353_207822325748
#define __DAVIS_H__0EDF3A92_5F95_42CA_A353_207822325748

#include <stdint.h>
#include <sys/types.h>

typedef struct {
	uint64_t t;
	uint16_t x, y;
	uint8_t parity;
	uint8_t id;
} polarity_event_t;


// TODO: define frame event
typedef struct {
	uint64_t t;
	uint8_t id;
} frame_event_t;


int socket_close(int sockfd);
int socket_open(const char *address, int port);
ssize_t socket_read(int sockfd, unsigned char *data, size_t n);
ssize_t socket_write(int sockfd, const char* data, size_t n);


#endif /* __DAVIS_H__0EDF3A92_5F95_42CA_A353_207822325748 */

