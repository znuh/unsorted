#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <linux/input.h>
#include <pthread.h>
#include <poll.h>
#include <stdio.h>
#include <errno.h>

typedef struct sn_axes_s {
	__s32 x, y, z;
	__s32 rx, ry, rz;
	__s32 btn_l, btn_r;
} sn_axes_t;

typedef struct spavenav_s {
	int fd;
	int quit;
	pthread_mutex_t mtx;
	pthread_t sn_thread;
	sn_axes_t axes;
} spacenav_t;

static void *sn_loop(void *priv) {
	spacenav_t *sn=priv;
	sn_axes_t *axes=&sn->axes;
	struct input_event evt;
	struct pollfd pfd;
	int res;
	
	pfd.fd = sn->fd;
	pfd.events=POLLIN;
	
	while(1) {
		// quit requested?
		pthread_mutex_lock(&sn->mtx);
		if(sn->quit) {
			pthread_mutex_unlock(&sn->mtx);
			break;
		}
		pthread_mutex_unlock(&sn->mtx);
		
		res = poll(&pfd, 1, 100);
		if(!res) continue;
		else if(res<0) {
			fprintf(stderr,"[spacenav] poll error: %s\n",strerror(errno));
			break;
		}
		
		res = read(sn->fd, &evt, sizeof(evt));
		if(res != sizeof(evt)) {
			fprintf(stderr,"[spacenav] read error: got %d, expected %ld\n",res,sizeof(evt));
			break;
		}
		pthread_mutex_lock(&sn->mtx);
		if(evt.type == EV_KEY) {
			if(evt.code == BTN_0)
				axes->btn_l = evt.value;
			else if(evt.code == BTN_1)
				axes->btn_r = evt.value;
		}
		else if(evt.type == EV_REL) {
			switch(evt.code) {
				case ABS_X:
					axes->x = evt.value;
					break;
				case ABS_Y:
					axes->y = evt.value;
					break;
				case ABS_Z:
					axes->z = evt.value;
					break;
				case ABS_RX:
					axes->rx = evt.value;
					break;
				case ABS_RY:
					axes->ry = evt.value;
					break;
				case ABS_RZ:
					axes->rz = evt.value;
					break;
			}
		}
		pthread_mutex_unlock(&sn->mtx);
		//printf("%x %x %d\n",evt.type, evt.code,evt.value);
	}
	
	return NULL;
}

spacenav_t *spacenav_create(char *dev) {
	spacenav_t *sn=NULL;
	int res = open(dev, O_RDONLY);
	
	if(res<0)
		return NULL;
	
	sn = malloc(sizeof(spacenav_t));
	bzero(sn, sizeof(spacenav_t));
	
	sn->fd = res;
	pthread_mutex_init(&sn->mtx, NULL);
	
	res = pthread_create(&sn->sn_thread, NULL, sn_loop, sn);
	if(res) {
		close(sn->fd);
		free(sn);
		return NULL;
	}
	
	return sn;
}

void spacenav_destroy(spacenav_t *sn) {
	pthread_mutex_lock(&sn->mtx);
	sn->quit=1;
	pthread_mutex_unlock(&sn->mtx);
	pthread_join(sn->sn_thread, NULL);
	close(sn->fd);
	free(sn);
}

void spacenav_get(spacenav_t *sn, sn_axes_t *axes) {
	pthread_mutex_lock(&sn->mtx);
	memcpy(axes, &sn->axes, sizeof(sn_axes_t));
	pthread_mutex_unlock(&sn->mtx);
}

#ifdef TEST_LIB
int main(int argc, char **argv) {
	spacenav_t *sn = spacenav_create("/dev/input/spacenavigator");
	if(sn) {
		sn_axes_t ax;
		printf("press both buttons to quit\n");
		do {
			usleep(10000);
			spacenav_get(sn, &ax);
			printf("\rx=%4d y=%4d z=%4d rx=%4d ry=%4d rz=%4d bl=%d br=%d",
				ax.x, ax.y, ax.z, ax.rx, ax.ry, ax.rz, ax.btn_l, ax.btn_r);
			fflush(stdout);
		} while(!((ax.btn_l == 1) && (ax.btn_r == 1)));
		printf("\n");
		spacenav_destroy(sn);
	}
	return 0;
}
#endif
