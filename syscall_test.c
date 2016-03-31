/*
 * Copyright (C) 2016 Benedikt Heinz <Zn000h AT gmail.com>
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
#include <inttypes.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

static uint32_t delta_t(const struct timeval *old, const struct timeval *now) {
	uint32_t res;
	if(now->tv_sec == old->tv_sec)
		res = now->tv_usec - old->tv_usec;
	else {
		res = 1000000 - old->tv_usec;
		res += now->tv_usec;
	}
	return res;
}

static void printtime(const char *test, const struct timeval *start, const struct timeval *end) {
	printf("%16s: %"PRIu32"\n", test, delta_t(start,end));
}

static void gtofd_test(void) {
	struct timeval tv1, tv2;
	gettimeofday(&tv1, NULL);
	gettimeofday(&tv2, NULL);
	printtime("gettimeofday",&tv1,&tv2);
}

static void usleep1_test(void) {
	struct timeval tv1, tv2;
	gettimeofday(&tv1, NULL);
	usleep(1);
	gettimeofday(&tv2, NULL);
	printtime("usleep(1)",&tv1,&tv2);
}

static void read4_test(void) {
	struct timeval tv1, tv2;
	int fd=open("/dev/zero",O_RDONLY);
	uint32_t val;
	int res;
	gettimeofday(&tv1, NULL);
	res=read(fd,&val,4);
	gettimeofday(&tv2, NULL);
	close(fd);
	res=res;
	printtime("read(4)",&tv1,&tv2);
}

static void write4_test(void) {
	struct timeval tv1, tv2;
	int fd=open("/dev/null",O_WRONLY);
	uint32_t val=23;
	int res;
	gettimeofday(&tv1, NULL);
	res=write(fd,&val,4);
	gettimeofday(&tv2, NULL);
	close(fd);
	res=res;
	printtime("write(4)",&tv1,&tv2);
}

/* NOTE: gettimeofday is no syscall or a special syscall on most systems
 * a timer is often mapped into the user address space and directly
 * read by the libc (see vdso(7)) instead of doing a full-fledged syscall
 * you don't see gettimeofday in strace then
 * 
 * some examples:
 * 
 * xubuntu 15.10 
 * 4.2.0-34-generic #39-Ubuntu SMP Thu Mar 10 22:13:01 UTC 2016 x86_64 x86_64 x86_64 GNU/Linux
 * Intel(R) Core(TM) i7-4600U CPU @ 2.10GHz (fam: 06, model: 45, stepping: 01)
 *   gettimeofday: 0
 *      usleep(1): 68
 *        read(4): 1
 *       write(4): 1
 * 
 * Debian GNU/Linux 8
 * 4.4.1 #9 SMP Thu Mar 17 16:26:26 CET 2016 armv7l GNU/Linux
 * i.MX6Q, silicon rev 1.2 (Cortex-A9 w/o vDSO gettimeofday)
 * ondemand cpufreq governor, 396MHz
 *   gettimeofday: 4
 *      usleep(1): 420
 *        read(4): 21
 *       write(4): 18
 */
int main(int argc, char **argv) {
	gtofd_test();
	usleep1_test();
	read4_test();
	write4_test();
	return 0;
}
