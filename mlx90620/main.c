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

void dump_ir(uint16_t ir_data[16][4]) {
	int x,y;
	for(y=0;y<4;y++) {
		for(x=0;x<16;x++) {
			printf("%04x ", ir_data[x][y]);
		}
		printf("\n");
	}
	printf("\n");
}

void convert_ir(uint16_t ir_data[16][4], double temp[16][4], uint8_t *eeprom, uint16_t ptat) {
	
}

// 10: init
// 23: eeprom map
// 29: commands

#define CFG_LSB	8 //0x0e //0x08u
#define CFG_MSB	4 //0x74 //0x04u

int config_mlx(int fd, uint8_t *eeprom) {
	uint8_t buf[16];
	
	// Write the oscillator trim value into the IO at address 0x93 (see 7.1, 8.2.2.2, 9.4.4)
	buf[0]=4u;
	buf[1]=(eeprom[0xF7]-0xAAu)&0xff;
	buf[2]=eeprom[0xF7];
	buf[3]=(0u-0xAAu)&0xff;
	buf[4]=0u;
	hexdump(buf,5);
	assert(write(fd,buf,5) == 5);

	// Write the configuration value (IO address 0x92) (see 8.2.2.1, 9.4.3)
	buf[0]=3u;
	buf[1]=(CFG_LSB-0x55u)&0xff; // LSB
	buf[2]=CFG_LSB;
	buf[3]=(CFG_MSB-0x55u)&0xff; // MSB
	buf[4]=CFG_MSB;
	hexdump(buf,5);
	assert(write(fd,buf,5) == 5);
	
	return 0;
}

enum {
	MLX_RAM_IR = 0,
	MLX_RAM_IR_END = 0x3f,
	MLX_RAM_PTAT = 0x90,
	MLX_RAM_TGC = 0x91,
	MLX_RAM_CONFIG = 0x92,
	MLX_RAM_TRIM = 0x93
};

int mlx_read_ram(int fd, uint8_t ofs, uint16_t *val, uint8_t n)
{
        int ret;
        struct i2c_rdwr_ioctl_data i2c_data;
        struct i2c_msg msg[2];
        uint8_t cmd[] = {2, 0xFF, 0, 1};
        
        cmd[1] = ofs;
        cmd[3] = n;
        
        i2c_data.msgs = msg;
        i2c_data.nmsgs = 2;     // two i2c_msg

        i2c_data.msgs[0].addr = 0x60;
        i2c_data.msgs[0].flags = 0;     // write
        i2c_data.msgs[0].len = 4;       
        i2c_data.msgs[0].buf = cmd;

		i2c_data.msgs[1].addr = 0x60;
        i2c_data.msgs[1].flags = I2C_M_RD;      // read command
        i2c_data.msgs[1].len = n*2;
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
	uint16_t ir_data[16][4];
	double temp[16][4];
	uint16_t cfg, ptat, trim;
	int fd;
	
	assert(argc>1);
	fd = open(argv[1], O_RDWR);
	assert(fd >= 0);
	
	read_eeprom(fd, eeprom);
	hexdump(eeprom, 255);
	
	assert(ioctl(fd, I2C_SLAVE, 0x60) >= 0);

	do {
		config_mlx(fd, eeprom);
	
		mlx_read_ram(fd, MLX_RAM_CONFIG, &cfg, 1);
		printf("cfg: %04x\n",cfg);
	} while(!(cfg&(1<<10)));
	
	mlx_read_ram(fd, MLX_RAM_TRIM, &trim, 1);
	printf("osc: %04x\n",trim);
	
	mlx_read_ram(fd, MLX_RAM_PTAT, &ptat, 1);
	printf("ptat: %04x\n",ptat);
	
	while(1) {
		mlx_read_ram(fd, MLX_RAM_IR, (uint16_t *)ir_data, 16*4);
		dump_ir(ir_data);
		convert_ir(ir_data, temp, eeprom, ptat);
	}
	
	close(fd);
	
	return 0;
}
