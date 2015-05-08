#include "davis.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <limits.h>


int
socket_close(int sockfd)
{
	int r = shutdown(sockfd, SHUT_RDWR);
	if(r != 0) {
		fprintf(stderr, "edvs_net_close: socket shutdown error %d\n", r);
		return -1;
	}
	r = close(sockfd);
	if(r != 0) {
		fprintf(stderr, "edvs_net_close: socket close error %d\n", r);
		return -1;
	}
	return 0;
}


int
socket_open(const char *address, int port)
{
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd <= 0) {
		char *err = strerror(errno);
		fprintf(stderr, "'%s'\n", err);
		return -1;
	}

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	int i = inet_pton(AF_INET, address, &addr.sin_addr);
	if(i <= 0) {
		fprintf(stderr, "edvs_net_open: inet p_ton error %d\n", i);
		return -1;
	}
	// connect
	if(connect(sockfd, (struct sockaddr*)&addr, sizeof(addr))) {
		fprintf(stderr, "edvs_net_open: connect error\n");
		return -1;
	}
	// return handle
	return sockfd;
}


ssize_t
socket_read(int sockfd, unsigned char* data, size_t n)
{
	ssize_t m = recv(sockfd, data, n, 0);
	if(m < 0) {
		fprintf(stderr, "edvs_net_read: recv error %zd\n", m);
		return -1;
	}
	return m;
}


ssize_t
socket_write(int sockfd, const char* data, size_t n)
{
	ssize_t m = send(sockfd, data, n, 0);
	if(m != n) {
		fprintf(stderr, "edvs_net_send: send error %zd\n", m);
	}
	return m;
}


