#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <stdio.h>
#include <alloca.h>
#include <assert.h>

int main(int argc, char **argv) {
	char buf[16];
	int fd,i=0;
	
	assert(argc>1);
	fd = open(argv[1], O_RDWR);
	assert(fd >= 0);
	
	assert(ioctl(fd, I2C_SLAVE, 0x50) >= 0);
	
	while(fgets(buf,16,stdin)) {
		uint8_t bla[2];
		int res;
		bla[0]=i;
		bla[1]=strtoul(buf,NULL,16);
		res=write(fd,bla,2);
		printf("%d: %d\n",i,res);
		i++;
	}
	
	close(fd);
	
	return 0;
}
