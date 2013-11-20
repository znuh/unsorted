/*
 * 
 * Copyright (C) 2013 Benedikt Heinz <Zn000h AT gmail.com>
 * 
 * This is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this code.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
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

#include "spacenav.h"

#define AXIS_VALUE_TIMEOUT	(16*2)	// milliseconds

static void *sn_loop(void *priv)
{
	spacenav_t *sn = priv;
	sn_axes_t *axes = &sn->axes;
	struct input_event evt;
	struct pollfd pfd;
	int res;

	pfd.fd = sn->fd;
	pfd.events = POLLIN;

	while (1) {
		// quit requested?
		pthread_mutex_lock(&sn->mtx);
		if (sn->quit) {
			pthread_mutex_unlock(&sn->mtx);
			break;
		}
		pthread_mutex_unlock(&sn->mtx);

		res = poll(&pfd, 1, 100);
		if (!res)
			continue;
		else if (res < 0) {
			pthread_mutex_lock(&sn->mtx);
			sn->errorcode = res;
			pthread_mutex_unlock(&sn->mtx);
			fprintf(stderr, "[spacenav] poll error: %s\n",
				strerror(errno));
			break;
		}

		res = read(sn->fd, &evt, sizeof(evt));
		if (res != sizeof(evt)) {
			pthread_mutex_lock(&sn->mtx);
			sn->errorcode = res;
			pthread_mutex_unlock(&sn->mtx);
			fprintf(stderr,
				"[spacenav] read error: got %d, expected %ld\n",
				res, sizeof(evt));
			break;
		}
		pthread_mutex_lock(&sn->mtx);
		if (evt.type == EV_KEY) {
			if (evt.code == BTN_0)
				axes->btn_l = evt.value;
			else if (evt.code == BTN_1)
				axes->btn_r = evt.value;
		} else if (evt.type == EV_REL) {
			switch (evt.code) {
			case ABS_X:
				axes->x = evt.value;
				sn->tm[0] = evt.time;
				break;
			case ABS_Y:
				axes->y = evt.value;
				sn->tm[1] = evt.time;
				break;
			case ABS_Z:
				axes->z = evt.value;
				sn->tm[2] = evt.time;
				break;
			case ABS_RX:
				axes->rx = evt.value;
				sn->tm[3] = evt.time;
				break;
			case ABS_RY:
				axes->ry = evt.value;
				sn->tm[4] = evt.time;
				break;
			case ABS_RZ:
				axes->rz = evt.value;
				sn->tm[5] = evt.time;
				break;
			}
		}
		pthread_mutex_unlock(&sn->mtx);
		//printf("%x %x %d\n",evt.type, evt.code,evt.value);
	}

	return NULL;
}

spacenav_t *spacenav_create(const char *dev)
{
	spacenav_t *sn = NULL;
	int res = open(dev, O_RDONLY);

	if (res < 0)
		return NULL;

	sn = malloc(sizeof(spacenav_t));
	bzero(sn, sizeof(spacenav_t));

	sn->fd = res;
	pthread_mutex_init(&sn->mtx, NULL);

	res = pthread_create(&sn->sn_thread, NULL, sn_loop, sn);
	if (res) {
		close(sn->fd);
		free(sn);
		return NULL;
	}

	return sn;
}

void spacenav_destroy(spacenav_t * sn)
{

	pthread_mutex_lock(&sn->mtx);
	sn->quit = 1;
	pthread_mutex_unlock(&sn->mtx);

	pthread_join(sn->sn_thread, NULL);

	close(sn->fd);
	free(sn);
}

static uint32_t tm_diff_ms(struct timeval *a, struct timeval *b)
{
	uint32_t diff = (a->tv_sec - b->tv_sec) * 1000;
	diff += (a->tv_usec / 1000);
	diff -= (b->tv_usec / 1000);
	return diff;
}

int spacenav_get(spacenav_t * sn, sn_axes_t * axes)
{
	struct timeval tm[6], now;
	int res;
	
	pthread_mutex_lock(&sn->mtx);
	memcpy(axes, &sn->axes, sizeof(sn_axes_t));
	memcpy(tm, sn->tm, sizeof(struct timeval) * 6);
	res = sn->errorcode;
	pthread_mutex_unlock(&sn->mtx);
	
	if(res) {
		bzero(axes, sizeof(sn_axes_t));
		return res;
	}
	
	gettimeofday(&now, NULL);
	if (tm_diff_ms(&now, tm) >= AXIS_VALUE_TIMEOUT)
		axes->x = 0;
	if (tm_diff_ms(&now, tm + 1) >= AXIS_VALUE_TIMEOUT)
		axes->y = 0;
	if (tm_diff_ms(&now, tm + 2) >= AXIS_VALUE_TIMEOUT)
		axes->z = 0;
	if (tm_diff_ms(&now, tm + 3) >= AXIS_VALUE_TIMEOUT)
		axes->rx = 0;
	if (tm_diff_ms(&now, tm + 4) >= AXIS_VALUE_TIMEOUT)
		axes->ry = 0;
	if (tm_diff_ms(&now, tm + 5) >= AXIS_VALUE_TIMEOUT)
		axes->rz = 0;
	
	return 0;
}

#ifdef TEST_LIB
int main(int argc, char **argv)
{
	spacenav_t *sn = spacenav_create("/dev/input/spacenavigator");
	if (sn) {
		sn_axes_t ax;
		printf("press both buttons to quit\n");
		do {
			usleep(10000);
			spacenav_get(sn, &ax);
			printf
			    ("\rx=%4d y=%4d z=%4d rx=%4d ry=%4d rz=%4d bl=%d br=%d",
			     ax.x, ax.y, ax.z, ax.rx, ax.ry, ax.rz, ax.btn_l,
			     ax.btn_r);
			fflush(stdout);
		} while (!((ax.btn_l == 1) && (ax.btn_r == 1)));
		printf("\n");
		spacenav_destroy(sn);
	}
	return 0;
}
#endif
