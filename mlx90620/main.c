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

struct mlx_conv_s {
	double v_th0;
	double d_Kt1;
	double d_Kt2;
	
	double tgc;
	double cyclops_alpha;
	double cyclops_A;
	double cyclops_B;
	double Ke;
	double KsTa;
};

double kelvin_to_celsius(double d) {
	return d - 273.15;
}

double ptat_to_kelvin(int16_t ptat, struct mlx_conv_s *conv) {
	double d = (conv->d_Kt1 * conv->d_Kt1 - 4 * conv->d_Kt2 * (1 - ptat / conv->v_th0));

	return ((-conv->d_Kt1 +  sqrt(d)) / (2 * conv->d_Kt2)) + 25 + 273.15;
}

void prepare_conv(uint8_t *eeprom, struct mlx_conv_s *conv) {
	uint8_t *Ai = eeprom;
	uint8_t *Bi = eeprom+0x40;
	uint8_t *da = eeprom+0x80;
	double Bi_scale;
	double alpha0_scale;
	double d_alpha_scale;
	double alpha_0;
	double d_common_alpha;
	
	conv->v_th0 = ((int16_t*)(eeprom+0xda))[0];
	
	conv->d_Kt1 = ((int16_t*)(eeprom+0xdc))[0];
	conv->d_Kt1 /= conv->v_th0;
	conv->d_Kt1 /= (1<<10);
	
	conv->d_Kt2 = ((int16_t*)(eeprom+0xde))[0];
	conv->d_Kt2 /= conv->v_th0;
	conv->d_Kt2 /= (1<<20);
	
	conv->tgc = ((int8_t *)eeprom+0xd8)[0];
	conv->tgc /= 32.0;
	
	conv->cyclops_alpha = ((uint16_t*)(eeprom+0xd6))[0];
	Bi_scale = pow(2, eeprom[0xD9]);
	
	conv->cyclops_A = ((int8_t*)eeprom)[0xd4];
	conv->cyclops_B = ((int8_t*)eeprom)[0xd5];
	conv->cyclops_B /= Bi_scale;
	
	alpha0_scale = pow(2, eeprom[0xe2]);
	d_alpha_scale = pow(2, eeprom[0xe3]);
	
	conv->Ke = ((uint16_t*)eeprom+0xE4)[0];
	conv->Ke /= 32768.0;
	
	conv->KsTa = ((int16_t*)eeprom+0xE6)[0];
	conv->KsTa /= pow(2, 20);
	
	alpha_0 = ((uint16_t*)eeprom+0xE0)[0];
	d_common_alpha = (alpha_0 - (conv->tgc * conv->cyclops_alpha)) / alpha0_scale;
	
	
}

int main(int argc, char **argv) {
	uint8_t eeprom[0xFF];
	uint16_t ir_data[16][4];
	struct mlx_conv_s conv_tbl;
	double temp[16][4];
	uint16_t cfg, ptat, trim;
	int fd;
	
	assert(argc>1);
	fd = open(argv[1], O_RDWR);
	assert(fd >= 0);
	
	read_eeprom(fd, eeprom);
	hexdump(eeprom, 255);
#if 0
	eeprom[0xda]=0x78;
	eeprom[0xdb]=0x1a;
	eeprom[0xdc]=0x33;
	eeprom[0xdd]=0x5b;
	eeprom[0xde]=0xcc;
	eeprom[0xdf]=0xed;
#endif
	prepare_conv(eeprom, &conv_tbl);
	
	assert(ioctl(fd, I2C_SLAVE, 0x60) >= 0);

	do {
		config_mlx(fd, eeprom);
	
		mlx_read_ram(fd, MLX_RAM_CONFIG, &cfg, 1);
		printf("cfg: %04x\n",cfg);
	} while(!(cfg&(1<<10)));
	
	mlx_read_ram(fd, MLX_RAM_TRIM, &trim, 1);
	printf("osc: %04x\n",trim);
	
	mlx_read_ram(fd, MLX_RAM_PTAT, &ptat, 1);
	//ptat=0x1ac0;
	printf("ptat: %04x (%.1f)\n",ptat,kelvin_to_celsius(ptat_to_kelvin((int16_t)ptat,&conv_tbl)));

	while(1) {
		mlx_read_ram(fd, MLX_RAM_IR, (uint16_t *)ir_data, 16*4);
		dump_ir(ir_data);
		convert_ir(ir_data, temp, eeprom, ptat);
	}
	
	close(fd);
	
	return 0;
}
