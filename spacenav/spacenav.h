#ifndef SPACENAV_H
#define SPACENAV_H

#include <sys/time.h>
#include <sys/types.h>
#include <linux/types.h>
#include <pthread.h>

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
	struct timeval tm[6];
} spacenav_t;

spacenav_t *spacenav_create(const char *dev);
void spacenav_destroy(spacenav_t * sn);
void spacenav_get(spacenav_t * sn, sn_axes_t * axes);

#endif
