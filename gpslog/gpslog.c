#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <time.h>
#include <fcntl.h>
#include <stdio.h>
#include <assert.h>
#include "rs232_if.h"

static int fd_nonblock(int sfd) {
	int res, flags = fcntl(sfd,F_GETFL,0);
	assert(flags != -1);
	res = fcntl(sfd, F_SETFL, flags | O_NONBLOCK);
	assert(!res);
	return res;
}

static char buf[4096];

int main(int argc, char **argv) {
	FILE *dst;
	int gps_fd=-1;
	char *port;
	int baudrate;
	char *dst_fn;
	struct pollfd pfd = {.fd=-1, .events=POLLIN|POLLERR|POLLHUP};
	struct timespec ts;
	
	assert(argc > 3);
	port = argv[1];
	baudrate = atoi(argv[2]);
	dst_fn = argv[3];
	
	dst = fopen(dst_fn, "a");
	assert(dst);
	
	while(1) {
		int res;
		if(gps_fd < 0) {
			gps_fd = rs232_init(port, baudrate, 0, 0);
			if(gps_fd>=0) {
				fd_nonblock(gps_fd);
				pfd.fd = gps_fd;
				puts("GPS connected");
			}
		}
		res = poll(&pfd, 1, 1000);
		clock_gettime(CLOCK_REALTIME, &ts);
		if(res < 0)
			continue;
		if(pfd.revents & (POLLERR|POLLHUP)) {
			close(gps_fd);
			gps_fd = -1;
			pfd.fd = -1;
			puts("GPS lost");
		}
		else if(pfd.revents & POLLIN) {
			res=read(gps_fd, buf, sizeof(buf));
			if(res > 0) {
				fprintf(dst, "%d.%ld\n",(int)ts.tv_sec, ts.tv_nsec);
				fwrite(buf, res, 1, dst);
				fflush(dst);
			}
			if(res >= 0)
				continue;
			close(gps_fd);
			gps_fd = -1;
			pfd.fd = -1;
			puts("GPS lost");
		}
	}
	
	return 0;
}
