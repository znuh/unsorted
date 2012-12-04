#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <alloca.h>
#include <assert.h>

#define SLAVE_ADDR		3
#define TUN_CAP_VAL		6
//#define OUTDOOR

typedef struct as3935_s {
	uint8_t pwd:1;
	uint8_t afe_gb:5;
	uint8_t rfu1:2;
	
	uint8_t wdth:4;
	uint8_t nf_lev:3;
	uint8_t rfu2:1;
	
	uint8_t srej:4;
	uint8_t min_num_ligh:2;
	uint8_t cl_stat:1;
	uint8_t rfu3:1;
	
	uint8_t ints:4;
	uint8_t rfu4:1;
	uint8_t mask_dist:1;
	uint8_t lco_fdiv:2;
	
	uint8_t s_lig_l:8;
	
	uint8_t s_lig_m:8;
	
	uint8_t s_lig_mm:5;
	uint8_t rfu5:3;
	
	uint8_t distance:6;
	uint8_t rfu6:2;
	
	uint8_t tun_cap:4;
	uint8_t rfu7:1;
	uint8_t disp_trco:1;
	uint8_t disp_srco:1;
	uint8_t disp_lco:1;
	
} as3935_t;

void print_regs(as3935_t *r) {
	printf("%12s: %d\n", "AFE_GB", r->afe_gb);
	printf("%12s: %d\n", "PWD", r->pwd);
	printf("%12s: %d\n", "NF_LEV", r->nf_lev);
	printf("%12s: %d\n", "WDTH", r->wdth);
	printf("%12s: %d\n", "CL_STAT", r->cl_stat);
	printf("%12s: %d\n", "MIN_NUM_LIGH", r->min_num_ligh);
	printf("%12s: %d\n", "SREJ", r->srej);
	printf("%12s: %d\n", "LCO_FDIV", r->lco_fdiv);
	printf("%12s: %d\n", "MASK_DIST", r->mask_dist);
	printf("%12s: %d\n", "INT", r->ints);
	printf("%12s: %d\n", "S_LIG_L", r->s_lig_l);
	printf("%12s: %d\n", "S_LIG_M", r->s_lig_m);
	printf("%12s: %d\n", "S_LIG_MM", r->s_lig_mm);
	printf("%12s: %d\n", "DISTANCE", r->distance);
	printf("%12s: %d\n", "DISP_LCO", r->disp_lco);
	printf("%12s: %d\n", "DISP_SRCO", r->disp_srco);
	printf("%12s: %d\n", "DISP_TRCO", r->disp_trco);
	printf("%12s: %d\n", "TUN_CAP", r->tun_cap);
}

void hexdump(const uint8_t *d, int l) {
	int i;
	printf("00 01 02 03 04 05 06 07 08\n");
	for(i=0;i<l;i++, d++) {
		printf("%02x ",*d);
	}
	printf("\n");
}

void dump_regs(int fd) {
	uint8_t regs[9] = {0};
	int res;
	
	res = write(fd, regs, 1);
	assert(res == 1);

	res = read(fd, regs, sizeof(regs));
	assert(res == sizeof(regs));
	
	hexdump(regs,sizeof(regs));
	print_regs((as3935_t*)regs);
}

int as3935_wreg(int fd, uint8_t reg, uint8_t val) {
	uint8_t buf[2] = {reg, val};
	return write(fd, buf, 2);
}

int as3935_rreg(int fd, uint8_t reg, uint8_t *val) {
	int res = write(fd, &reg, 1);
	assert(res == 1);
	return read(fd, val, 1);
}

#define DIRECT_COMMAND	0x96

int as3935_init(int fd) {
	as3935_wreg(fd, 0x3c, DIRECT_COMMAND); // preset default
	as3935_wreg(fd, 0x3d, DIRECT_COMMAND); // calib_rco
	as3935_wreg(fd, 0x08, (1<<5));
	usleep(3000);
	as3935_wreg(fd, 0x08, 0);
	as3935_wreg(fd, 0x08, TUN_CAP_VAL);
#ifdef OUTDOOR
	as3935_wreg(fd, 0, (14<<1));
#endif
	return 0;
}

int main(int argc, char **argv) {
	int fd, res;
	
	assert(argc>1);
	fd = open(argv[1], O_RDWR);
	assert(fd >= 0);
	
	assert(ioctl(fd, I2C_SLAVE, SLAVE_ADDR) >= 0);

	as3935_init(fd);

	dump_regs(fd);
	
	if(argc>2) {
		uint8_t *wrbuf=alloca(argc-2);
		int i;
		printf("write: ");
		for(i=2;i<argc;i++) {
			wrbuf[i-2]=strtoul(argv[i],NULL,0)&0xff;
			printf("0x%02x ",wrbuf[i-2]);
		}
		printf("\n");
		res = write(fd, wrbuf, argc-2);
		assert(res == (argc-2));
		dump_regs(fd);
	}
	
	close(fd);
	
	return 0;
}
