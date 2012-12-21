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
	assert(read(fd, d, 0xFF) == 0xFF);
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

void hexdump16(uint8_t *d, int l) {
	int i;
	for(i=0;i<l;i++) {
		if(!(i&15)) printf("%3x: ",i);
		printf("%02x%02x ",d[0],d[1]);
		d+=2;
		if((i&15) == 15) printf("\n");
	}
	printf("\n");
}

// 10: init
// 23: eeprom map
// 29: commands

#define CFG_LSB	0x0e //0x08u
#define CFG_MSB	0x74 //0x04u

int config_mlx(int fd, uint8_t *eeprom) {
	uint8_t buf[16];
	
	// Write the oscillator trim value into the IO at address 0x93 (see 7.1, 8.2.2.2, 9.4.4)
	buf[0]=4u;
	buf[1]=eeprom[0xF7]-0xAAu;
	buf[2]=eeprom[0xF7];
	buf[3]=0u-0xAAu;
	buf[4]=0u;
	hexdump(buf,5);
	assert(write(fd,buf,5) == 5);

	// Write the configuration value (IO address 0x92) (see 8.2.2.1, 9.4.3)
	buf[0]=3u;
	buf[1]=CFG_LSB-0x55u; // LSB
	buf[2]=CFG_LSB;
	buf[3]=CFG_MSB-0x55u; // MSB
	buf[4]=CFG_MSB;
	hexdump(buf,5);
	assert(write(fd,buf,5) == 5);
	
	return 0;
}

enum {
	MLX_RAM_PTAT = 0x90,
	MLX_RAM_TGC = 0x91,
	MLX_RAM_CONFIG = 0x92,
	MLX_RAM_TRIM = 0x93
};

int mlx_read_reg(int fd, uint8_t reg, uint16_t *val)
{
        int ret;
        struct i2c_rdwr_ioctl_data i2c_data;
        struct i2c_msg msg[2];
        uint8_t cmd[] = {2, 0xFF, 0, 1};
        
        cmd[1] = reg;
        
        i2c_data.msgs = msg;
        i2c_data.nmsgs = 2;     // two i2c_msg

        i2c_data.msgs[0].addr = 0x60;
        i2c_data.msgs[0].flags = 0;     // write
        i2c_data.msgs[0].len = 4;       
        i2c_data.msgs[0].buf = cmd;

		i2c_data.msgs[1].addr = 0x60;
        i2c_data.msgs[1].flags = I2C_M_RD;      // read command
        i2c_data.msgs[1].len = 2;
        i2c_data.msgs[1].buf = (uint8_t *) val;

        ret = ioctl(fd, I2C_RDWR, &i2c_data);

        if (ret < 0) {
                perror("read data fail\n");
                return ret;
        }

        return 0;
}

int main(int argc, char **argv) {
	uint8_t eeprom[0xFF];
	uint16_t ir_data[16*4];
	uint16_t val;
	int fd, res;
	
	assert(argc>1);
	fd = open(argv[1], O_RDWR);
	assert(fd >= 0);
	
	read_eeprom(fd, eeprom);
	//hexdump(eeprom, 255);
	
	assert(ioctl(fd, I2C_SLAVE, 0x60) >= 0);

	do {
		config_mlx(fd, eeprom);
	
		mlx_read_reg(fd, MLX_RAM_CONFIG, &val);
		printf("cfg: %04x\n",val);
	} while(!(val&(1<<10)));
	
	mlx_read_reg(fd, MLX_RAM_TRIM, &val);
	printf("osc: %04x\n",val);
	
	mlx_read_reg(fd, MLX_RAM_PTAT, &val);
	printf("ptat: %04x\n",val);
	
	while(0) {
	}
	
	close(fd);
	
	return 0;
}
