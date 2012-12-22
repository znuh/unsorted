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
#include <SDL/SDL.h>
#include <SDL/SDL_gfxPrimitives.h>
#include <SDL/SDL_ttf.h>

#define PIX_SIZE	64

static TTF_Font *font = NULL;

SDL_Surface *sdl_init(void)
{
	Uint8 video_bpp;
	Uint32 videoflags;
	SDL_Surface *screen;
	int w = PIX_SIZE * 16, h = PIX_SIZE * 5;
	    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "can't initialize SDL video\n");
		exit(1);
	}
	atexit(SDL_Quit);

	TTF_Init();
	font =
	    TTF_OpenFont
	    ("/usr/share/fonts/truetype/ttf-dejavu/DejaVuSansMono.ttf", 18);
	assert(font);

	video_bpp = 32;
	videoflags = SDL_HWSURFACE | SDL_DOUBLEBUF /*| SDL_FULLSCREEN */ ;

	if ((screen = SDL_SetVideoMode(w, h, video_bpp, videoflags)) == NULL) {
		fprintf(stderr, "can't set video mode %dx%d\n", w, h);
		exit(2);
	}

	SDL_WM_SetCaption("thermoview", "thermoview");

	return screen;
}

Uint32 color_from_temp(double t, double maxval, double minval) {
	Uint32 res=0x000080ff;
	double f = maxval - minval;
	f = 256/f;
	t-=minval;
	res |= ((int)(t*f)&0xff)<<24;
	res |= ((int)(t*f)&0xff)<<16;
	return res;
}

void draw_picture(SDL_Surface * sf, double temps[16][4])
{
	int x, y;
	double maxval=-999999999, minval=999999999;
	char buf[16];
	SDL_Color fg = { 0, 0, 0 };
	SDL_Rect rect;
	SDL_Surface *txt_sf;
	SDL_FillRect(sf,NULL, 0);
	for (y = 0; y < 4; y++) {
		for (x = 0; x < 16; x++) {
			if(temps[x][y]>maxval)
				maxval = temps[x][y];
			if(temps[x][y]<minval)
				minval = temps[x][y];
		}
	}
	for (y = 0; y < 4; y++) {
		for (x = 0; x < 16; x++) {
			boxColor(sf, x * PIX_SIZE, y * PIX_SIZE,
				       x * PIX_SIZE + PIX_SIZE - 1,
				       y * PIX_SIZE + PIX_SIZE - 1,
				       color_from_temp(temps[x][y],maxval,minval));
			sprintf(buf,"%3.1f",temps[x][y]);
			txt_sf = TTF_RenderText_Blended(font, buf, fg);
			rect.w=txt_sf->w;
			rect.h=txt_sf->h;
			rect.x=x*PIX_SIZE+10;
			rect.y=y*PIX_SIZE+20;
			SDL_BlitSurface(txt_sf,NULL,sf,&rect);
			SDL_FreeSurface(txt_sf);
		}
	}
	for(x=0;x<256;x++) {
		vlineColor(sf,x+80,PIX_SIZE*4,PIX_SIZE*4+PIX_SIZE, 0x80ff + (x<<16) + (x<<24));
	}
	
	fg.r = fg.g = fg.b = 0xff;
	
	sprintf(buf,"%3.1f",minval);
	txt_sf = TTF_RenderText_Blended(font, buf, fg);
	rect.w=txt_sf->w;
	rect.h=txt_sf->h;
	rect.x=10;
	rect.y=4*PIX_SIZE+30;
	SDL_BlitSurface(txt_sf,NULL,sf,&rect);
	SDL_FreeSurface(txt_sf);
	
	sprintf(buf,"%3.1f",maxval);
	txt_sf = TTF_RenderText_Blended(font, buf, fg);
	rect.w=txt_sf->w;
	rect.h=txt_sf->h;
	rect.x=80+256+10;
	rect.y=4*PIX_SIZE+30;
	SDL_BlitSurface(txt_sf,NULL,sf,&rect);
	SDL_FreeSurface(txt_sf);
	
	SDL_Flip(sf);

}

int read_eeprom(int fd, uint8_t * d)
{
	assert(ioctl(fd, I2C_SLAVE, 0x50) >= 0);
	*d = 0;
	assert(write(fd, d, 1) == 1);
	assert(read(fd, d, 0xFF) == 0xFF);
	return 0;
}

void hexdump(uint8_t * d, int l)
{
	int i;
	for (i = 0; i < l; i++, d++) {
		if (!(i & 15))
			printf("%02x: ", i);
		printf("%02x%s", *d, ((i & 15) == 15) ? "\n" : " ");
	}
	printf("\n");
}

void dump_ir(int16_t ir_data[16][4])
{
	int x, y;
	for (y = 0; y < 4; y++) {
		for (x = 0; x < 16; x++) {
			printf("%04x ", (uint16_t) ir_data[x][y]);
		}
		printf("\n");
	}
	printf("\n");
}

void dump_temps(double temps[16][4])
{
	int x, y;
	for (y = 0; y < 4; y++) {
		for (x = 0; x < 16; x++) {
			//temps[x][y]=y*16+x;
			printf("%3.1f ", temps[x][y]);
		}
		printf("\n");
	}
	printf("\n");
}

// 10: init
// 23: eeprom map
// 29: commands

#define CFG_LSB	0x0au		//0x0e //0x08u
#define CFG_MSB	0x74u		//0x04u

int config_mlx(int fd, uint8_t * eeprom)
{
	uint8_t buf[16];

	// Write the oscillator trim value into the IO at address 0x93 (see 7.1, 8.2.2.2, 9.4.4)
	buf[0] = 4u;
	buf[1] = (eeprom[0xF7] - 0xAAu) & 0xff;
	buf[2] = eeprom[0xF7];
	buf[3] = (0u - 0xAAu) & 0xff;
	buf[4] = 0u;
	hexdump(buf, 5);
	assert(write(fd, buf, 5) == 5);

	// Write the configuration value (IO address 0x92) (see 8.2.2.1, 9.4.3)
	buf[0] = 3u;
	buf[1] = (CFG_LSB - 0x55u) & 0xff;	// LSB
	buf[2] = CFG_LSB;
	buf[3] = (CFG_MSB - 0x55u) & 0xff;	// MSB
	buf[4] = CFG_MSB;
	hexdump(buf, 5);
	assert(write(fd, buf, 5) == 5);

	return 0;
}

enum {
	MLX_RAM_IR = 0x00,
	MLX_RAM_IR_END = 0x3f,
	MLX_RAM_PTAT = 0x90,
	MLX_RAM_TGC = 0x91,
	MLX_RAM_CONFIG = 0x92,
	MLX_RAM_TRIM = 0x93
};

int mlx_read_ram(int fd, uint8_t ofs, uint16_t * val, uint8_t n)
{
	int ret;
	struct i2c_rdwr_ioctl_data i2c_data;
	struct i2c_msg msg[2];
	uint8_t cmd[] = { 2, 0xFF, 0, 1 };

	cmd[1] = ofs;
	cmd[2] = (n > 1) ? 1 : 0;
	cmd[3] = n;

	i2c_data.msgs = msg;
	i2c_data.nmsgs = 2;	// two i2c_msg

	i2c_data.msgs[0].addr = 0x60;
	i2c_data.msgs[0].flags = 0;	// write
	i2c_data.msgs[0].len = 4;
	i2c_data.msgs[0].buf = cmd;

	i2c_data.msgs[1].addr = 0x60;
	i2c_data.msgs[1].flags = I2C_M_RD;	// read command
	i2c_data.msgs[1].len = n * 2;
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

	double Ai[16 * 4];
	double Bi[16 * 4];
	double alpha_i[16 * 4];
};

double kelvin_to_celsius(double d)
{
	return d - 273.15;
}

double ptat_to_kelvin(double ptat, struct mlx_conv_s *conv)
{
	double d =
	    (conv->d_Kt1 * conv->d_Kt1 -
	     4 * conv->d_Kt2 * (1 - ptat / conv->v_th0));

	return ((-conv->d_Kt1 + sqrt(d)) / (2 * conv->d_Kt2)) + 25 + 273.15;
}

void prepare_conv(uint8_t * eeprom, struct mlx_conv_s *conv)
{
	int8_t *Ai = (int8_t *) eeprom;
	int8_t *Bi = (int8_t *) eeprom + 0x40;
	uint8_t *da = eeprom + 0x80;
	double Bi_scale;
	double alpha0_scale;
	double d_alpha_scale;
	double alpha_0;
	double d_common_alpha;
	int i;

	conv->v_th0 = ((int16_t *) (eeprom + 0xda))[0];

	conv->d_Kt1 = ((int16_t *) (eeprom + 0xdc))[0];
	conv->d_Kt1 /= conv->v_th0;
	conv->d_Kt1 /= (1 << 10);

	conv->d_Kt2 = ((int16_t *) (eeprom + 0xde))[0];
	conv->d_Kt2 /= conv->v_th0;
	conv->d_Kt2 /= (1 << 20);

	conv->tgc = ((int8_t *) eeprom + 0xd8)[0];
	conv->tgc /= 32.0;

	conv->cyclops_alpha = ((uint16_t *) (eeprom + 0xd6))[0];
	Bi_scale = pow(2, eeprom[0xD9]);

	conv->cyclops_A = ((int8_t *) eeprom)[0xd4];
	conv->cyclops_B = ((int8_t *) eeprom)[0xd5];
	conv->cyclops_B /= Bi_scale;

	alpha0_scale = pow(2, eeprom[0xe2]);
	d_alpha_scale = pow(2, eeprom[0xe3]);

	conv->Ke = ((uint16_t *) eeprom + 0xE4)[0];
	conv->Ke /= 32768.0;

	conv->KsTa = ((int16_t *) eeprom + 0xE6)[0];
	conv->KsTa /= pow(2, 20);

	alpha_0 = ((uint16_t *) eeprom + 0xE0)[0];
	d_common_alpha =
	    (alpha_0 - (conv->tgc * conv->cyclops_alpha)) / alpha0_scale;

	for (i = 0; i < 16 * 4; i++) {
		conv->Ai[i] = Ai[i];
		conv->Bi[i] = Bi[i];
		conv->Bi[i] /= Bi_scale;
		conv->alpha_i[i] = da[i];
		conv->alpha_i[i] /= d_alpha_scale;
		conv->alpha_i[i] += d_common_alpha;
		printf("xx %f %f %f %f %f\n", conv->Ai[i], conv->Bi[i],
		       conv->alpha_i[i], d_alpha_scale, d_common_alpha);
	}
}

void convert_ir(int16_t ir_data[16][4], double temp[16][4],
		struct mlx_conv_s *conv, double ptat, int16_t tgc)
{
	double cyclops_val =
	    conv->tgc * ((double)tgc -
			 (conv->cyclops_A +
			  conv->cyclops_B * (kelvin_to_celsius(ptat) - 25)));
	int i, x, y;
	double ta_corr = pow(ptat, 4);

	for (x = 0, i = 0; x < 16; x++) {
		for (y = 0; y < 4; y++, i++) {
			double v_in = ir_data[x][y];
			//printf("%f\n",v_in);
			printf("%f %f %f\n", conv->Ai[i], conv->Bi[i],
			       conv->alpha_i[i]);
			//printf("%f\n",(1 + conv->KsTa * (kelvin_to_celsius(ptat) - 35)));
			//printf("%f\n",conv->Ai[i] + conv->Bi[i] * (kelvin_to_celsius(ptat)));
			//printf("%f %f\n",conv->Ke ,conv->alpha_i[i]);
			//(1 + conv->KsTa * (kelvin_to_celsius(ptat) - 35))));
			double v =
			    (v_in -
			     (conv->Ai[i] +
			      conv->Bi[i] * (kelvin_to_celsius(ptat) - 25))
			     -
			     cyclops_val) / (conv->Ke * conv->alpha_i[i] * (1 +
									    conv->
									    KsTa
									    *
									    (kelvin_to_celsius
									     (ptat)
									     -
									     35)))
			    + ta_corr;
			temp[x][y] = pow(v, 0.25);
			temp[x][y] = ir_data[x][y];
		}
	}
}

int main(int argc, char **argv)
{
	uint8_t eeprom[0xFF];
	int16_t ir_data[16][4];
	struct mlx_conv_s conv_tbl;
	double temp[16][4];
	uint16_t cfg, ptat, trim;
	int16_t tgc;
	double ptat_f;
	SDL_Surface *screen;
	int fd;
	int quit = 0;

	assert(argc > 1);
	fd = open(argv[1], O_RDWR);
	assert(fd >= 0);

	read_eeprom(fd, eeprom);
	hexdump(eeprom, 255);
#if 0
	eeprom[0xda] = 0x78;
	eeprom[0xdb] = 0x1a;
	eeprom[0xdc] = 0x33;
	eeprom[0xdd] = 0x5b;
	eeprom[0xde] = 0xcc;
	eeprom[0xdf] = 0xed;
#endif
	prepare_conv(eeprom, &conv_tbl);

	assert(ioctl(fd, I2C_SLAVE, 0x60) >= 0);

	do {
		config_mlx(fd, eeprom);

		mlx_read_ram(fd, MLX_RAM_CONFIG, &cfg, 1);
		printf("cfg: %04x\n", cfg);
	} while (!(cfg & (1 << 10)));

	mlx_read_ram(fd, MLX_RAM_TRIM, &trim, 1);
	printf("osc: %04x\n", trim);

	screen = sdl_init();

	while (!quit) {
		SDL_Event evt;
		mlx_read_ram(fd, MLX_RAM_TGC, (uint16_t *) & tgc, 1);
		printf("tgc: %04x\n", (uint16_t) tgc);

		mlx_read_ram(fd, MLX_RAM_PTAT, &ptat, 1);
		ptat_f = kelvin_to_celsius(ptat_to_kelvin(ptat, &conv_tbl));
		//ptat=0x1ac0;
		printf("ptat: %04x (%.1f)\n", ptat, ptat_f);

		mlx_read_ram(fd, MLX_RAM_IR, (uint16_t *) ir_data, 16 * 4);
		dump_ir(ir_data);
		convert_ir(ir_data, temp, &conv_tbl, ptat_f, tgc);
		dump_temps(temp);
		draw_picture(screen, temp);
		while (SDL_PollEvent(&evt)) {
			if (evt.type == SDL_KEYDOWN) {
				switch (evt.key.keysym.sym) {
				case SDLK_q:
				case SDLK_ESCAPE:
					quit = 1;
					break;
				default:
				break;
				}
			}

		}
	}

	close(fd);

	return 0;
}
