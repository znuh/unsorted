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

#define SLAVE_ADDR		0x60

void hexdump(const uint8_t *d, int l) {
	int i;
	printf("00 01 02 03 04 05 06 07 08\n");
	for(i=0;i<l;i++, d++) {
		printf("%02x ",*d);
	}
	printf("\n");
}

#define SI1132_CMD_RESET		0x01
#define SI1132_CMD_ALS_AUTO		0x0E
#define SI1132_CMD_SET			0xA0
#define SI1132_CMD_QUERY		0x80

#define SI1132_REG_HW_KEY		0x07
#define SI1132_HW_KEY			0x17

#define SI1132_REG_PARAM_WR		0x17
#define SI1132_REG_CMD			0x18
#define SI1132_REG_RESPONSE		0x20

#define SI1132_REG_PARAM_RD		0x2E
#define SI1132_REG_CHIP_STAT	0x30

#define SI1132_PARAM_CHLIST		0x01
#define SI1132_PARAM_CHLIST_MASK	0xf0

#define SI1132_REG_MEAS_RATE0	0x08
#define SI1132_REG_MEAS_RATE1	0x09

#define SI1132_REG_UCOEF0		0x13
#define SI1132_REG_UCOEF1		0x14
#define SI1132_REG_UCOEF2		0x15
#define SI1132_REG_UCOEF3		0x16

/*

  // fastest clocks
  	writeParam(Si1132_PARAM_ALSVISADCGAIN, 3);
  // take 511 clocks to measure
  	writeParam(Si1132_PARAM_ALSVISADCCOUNTER, Si1132_PARAM_ADCCOUNTER_511CLK);
  // in high range mode (not normal signal)
  	//writeParam(Si1132_PARAM_ALSVISADCMISC, Si1132_PARAM_ALSVISADCMISC_VISRANGE);
	* */

const int init_regs[] = {
	SI1132_REG_MEAS_RATE0, 	0x20,
	SI1132_REG_UCOEF0, 		0x7b,
	SI1132_REG_UCOEF1, 		0x6b,
	SI1132_REG_UCOEF2, 		0x01,
	SI1132_REG_UCOEF3, 		0x00,
	-1, -1
};

#define SI1132_PARAM_ALS_IR_ADCMUX			0x0e
#define SI1132_PARAM_ALS_IR_ADC_GAIN		0x1e
#define SI1132_PARAM_ALS_IR_ADC_COUNTER		0x1d
#define SI1132_PARAM_ALS_IR_ADC_MISC		0x1f
#define SI1132_PARAM_ALS_VIS_ADC_GAIN		0x11
#define SI1132_PARAM_ALS_VIS_ADC_COUNTER	0x10
#define SI1132_PARAM_ALS_VIS_ADC_MISC		0x12

const int init_parms[] = {
	SI1132_PARAM_CHLIST, 			SI1132_PARAM_CHLIST_MASK,
	SI1132_PARAM_ALS_IR_ADCMUX, 	0,
	
	SI1132_PARAM_ALS_IR_ADC_GAIN,				0,
	SI1132_PARAM_ALS_IR_ADC_COUNTER,				0x70,
	SI1132_PARAM_ALS_IR_ADC_MISC,				0x20,
	
	SI1132_PARAM_ALS_VIS_ADC_GAIN,				3,
	SI1132_PARAM_ALS_VIS_ADC_COUNTER,				0x70,
	SI1132_PARAM_ALS_VIS_ADC_MISC,				0x20,
	
	-1, -1
};

int si1132_write_reg(int fd, int reg, int data) {
	uint8_t buf[2] = {reg&0xff, data&0xff};
	int res = write(fd,buf,2);
	assert(res == 2);
	return 0;
}

int si1132_read_reg(int fd, int reg) {
	uint8_t buf[1] = {reg&0xff};
	int res = write(fd,buf,1);
	assert(res == 1);
	res = read(fd,buf,1);
	assert(res == 1);
	return buf[0];
}

int si1132_read_reg16(int fd, int reg) {
	uint8_t buf[2] = {reg&0xff};
	int res = write(fd,buf,1);
	assert(res == 1);
	res = read(fd,buf,2);
	assert(res == 2);
	return (buf[1]<<8)|buf[0];
}

int si1132_cmd(int fd, int cmd, int param) {
	int res;
	retry:
	si1132_write_reg(fd, SI1132_REG_CMD, 0);
	si1132_write_reg(fd, SI1132_REG_PARAM_WR, param&0xff);
	si1132_write_reg(fd, SI1132_REG_CMD, cmd&0xff);
	usleep(25000);
	res = si1132_read_reg(fd, SI1132_REG_RESPONSE);
	assert(res>=0);
	//printf("cmd %x resp %x\n",cmd,res);
	//if(!res)
		//goto retry;
	return 0;
}

void si1132_read(int fd, float *uv, float *ir, float *vis) {
	*uv = si1132_read_reg16(fd, 0x2c) / 100;
	*ir = si1132_read_reg16(fd, 0x24);
	*vis = si1132_read_reg16(fd, 0x22);
}

int si1132_init(int fd) {
	int i;
	
	si1132_write_reg(fd,SI1132_REG_HW_KEY, SI1132_HW_KEY);
	//si1132_cmd(fd, SI1132_CMD_RESET, 0);
	//usleep(10000);
	//si1132_cmd(fd, 0, 0);
	for(i=0;init_regs[i]>=0;i+=2) {
		si1132_write_reg(fd, init_regs[i], init_regs[i+1]);
	}
	for(i=0;init_parms[i]>=0;i+=2) {
		si1132_cmd(fd, SI1132_CMD_SET | init_parms[i], init_parms[i+1]);
	}
	//si1132_cmd(fd, SI1132_CMD_SET | SI1132_PARAM_CHLIST, SI1132_PARAM_CHLIST_MASK);
	//si1132_cmd(fd, SI1132_CMD_ALS_AUTO, 0);
	//si1132_cmd(fd, 0x12, 0);
	si1132_cmd(fd, 0x06, 0);
	usleep(100000);
	return 0;
}

void dump_regs(int fd) {
	uint8_t buf[0x40] = {0};
	int i,res;
	write(fd,buf,1);
	res = read(fd,buf,0x3e);
	assert(res == 0x3e);
	for(i=0;i<res;i++) {
		printf("%2x: %02x\n",i,buf[i]);
	}
}

int main(int argc, char **argv) {
	int fd, res;
	
	assert(argc>1);
	fd = open(argv[1], O_RDWR);
	assert(fd >= 0);
	
	assert(ioctl(fd, I2C_SLAVE, SLAVE_ADDR) >= 0);

	si1132_init(fd);

	dump_regs(fd);
	
	while(1) {
		float uv, ir, vis;
		si1132_read(fd,&uv,&ir,&vis);
		si1132_cmd(fd,0x06,0);
		printf("%4.1f %4.1f %4.1f\n",uv,ir,vis);
		usleep(500000);
	}
	
	close(fd);
	
	return 0;
}
