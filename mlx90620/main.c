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
#include <stdio.h>
#include <alloca.h>
#include <assert.h>

int read_eeprom(int fd, uint8_t *d) {
	assert(ioctl(fd, I2C_SLAVE, 0x50) >= 0);
	*d=0;
	assert(write(fd, d, 1) == 1);
	assert(read(fd, d, 0xF9) == 0xF9);
	return 0;
}

void hexdump(uint8_t *d, int l) {
	int i;
	for(i=0;i<l;i++,d++) {
		if(!(i&15)) printf("%02x: ",i);
		printf("%02x%s",*d,((i&15) == 15) ? "\n" : " ");
	}
	printf("\n");
}

int main(int argc, char **argv) {
	uint8_t eeprom[0xF9];
	int fd, res;
	
	assert(argc>1);
	fd = open(argv[1], O_RDWR);
	assert(fd >= 0);
	
	read_eeprom(fd, eeprom);
	hexdump(eeprom, 255);
	
	close(fd);
	
	return 0;
}
