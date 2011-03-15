/*
 * 
 * TODO:
 *
 * - handle overflows (>65k cpm / >655 uSv/h)
 * - dead time compensation
 * - ui w/ input
 *    - switch timeframe (n minutes)
 *    - sv_factor setting (calibration)
 *    - accumulated dose mode
 * - time-to-count: http://en.wikipedia.org/wiki/Dead_time#Time-To-Count
 *
 * dead time is ~190us -> max. 5.2k cps / 316k cpm -> 3.2 mSv/h w/o compensation
 *
 */

#define F_CPU	8000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>

#define DDR_SPI 	DDRB
#define DD_MOSI 	PB5
#define DD_SCK 		PB7
#define DD_nCS		PB4
#define DD_RS		PB2

#define DOGM_CMD_MASK		0xf0
#define DOGM_CMD_FOLLOW		0x60

#define DOGM_INSTR	{ PORTB &= ~((1<<DD_nCS) | (1<<DD_RS)); }
#define DOGM_DATA 	{ PORTB &= ~(1<<DD_nCS); }
#define DOGM_DONE	{ PORTB |= (1<<DD_nCS) | (1<<DD_RS); }

/* "dogm" = EA DOGM163 LCD */

static void dogm_clear(void) {

	DOGM_INSTR;

	SPDR = 1;
	_delay_ms(2);

	DOGM_DONE;
	
}

static void dogm_cursor(uint8_t y, uint8_t x) {
	DOGM_INSTR;
	SPDR = 0x80 + ((y&3)<<4) + (x&15);
	_delay_us(30);
	DOGM_DONE;
}

static void dogm_print(char *buf) {
	DOGM_DATA;
	
	for(;*buf;buf++) {
		SPDR = *buf;
		_delay_us(30);
	}
	DOGM_DONE;	
}
	
static void init(void) {
	uint8_t dogm_init[] = {
		0x38, // function set
		0x39, // function set
		0x1d, // bias ??
		0x7c, // contrast
		0x50, // power/icon/contrast: booster off
		0x6c, // follower off
		0x0c, // on/off ctl
	};
	uint8_t i;
	
	// nCS hi, data mode
	PORTB = (1<<DD_nCS) | (1<<DD_RS);

	/* Set MOSI and SCK output, all others input */
	DDRB = (1<<DD_MOSI)|(1<<DD_SCK) | (1<<DD_nCS) | (1<<DD_RS);
	
	/* Enable SPI, Master, set clock rate fck/16 */
	SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR0);
	
	_delay_ms(50);

	DOGM_INSTR;
	
	for(i=0; i<sizeof(dogm_init); i++) {
		uint8_t val = dogm_init[i];
		
		SPDR = val;
		
		// follower control needs 200 ms until power stable
		if((val & DOGM_CMD_MASK) == DOGM_CMD_FOLLOW) {
			_delay_ms(250);
		}
		else {
			_delay_us(30);
		}
	}

	DOGM_DONE;

	dogm_clear();
}

static volatile uint8_t ovf_cnt = 0;

static volatile uint8_t update = 0;

static volatile uint16_t sec10_count = 0;
static volatile uint16_t sec60_count = 0;

static volatile uint16_t sample_buf[60];
static volatile uint8_t sec10_idx = 60-10;
static volatile uint8_t sec60_idx = 0;

static volatile uint8_t rt = 0;

static volatile uint16_t last_cnt = 0;

ISR(TIMER0_OVF_vect) {

	ovf_cnt++;

	if(ovf_cnt == 30) { // actually: 125 ns * 1024 * 256 * 30 = 983ms
		uint16_t delta_cnt, new_cnt = TCNT1;
		
		// FIXME: handle (new == last) w/ overrun case
		if(new_cnt >= last_cnt)
			delta_cnt = new_cnt - last_cnt;
		else {
			delta_cnt = 0xffff - last_cnt;
			delta_cnt += new_cnt + 1;
		}
		
		last_cnt = new_cnt;
		
		sec10_count -= sample_buf[sec10_idx];
		sec60_count -= sample_buf[sec60_idx];
		
		sec10_count += delta_cnt;
		sec60_count += delta_cnt;
		
		sample_buf[sec60_idx++] = delta_cnt;
		if(sec60_idx >= 60)
			sec60_idx = 0;
		
		sec10_idx++;
		if(sec10_idx >= 60)
			sec10_idx = 0;
		
		rt++;
		if(!rt)
			rt=255;
	
		update++;
		ovf_cnt = 0;
	}
}

static void u16_to_dec(uint16_t val, char *buf, uint8_t pt_pos) {
	uint8_t i, n = pt_pos ? 6 : 5;
	uint8_t zeroes = pt_pos + 1;

	buf+= n - 1;
	
	for(i=0;i<n;i++,buf--) {
		char c = zeroes ? '0' : ' ';

		if((pt_pos) && (i == pt_pos)) {
			c = '.';
			zeroes=2;
		}
		else if(val) {
			uint16_t new = val/10;
			c = val - (new * 10) + '0';		
			val = new;
		}
		if(zeroes)
			zeroes--;
		*buf = c;
	}
}

int main(void) {
	char txtbuf[16*3+1]="       cps             cpm             uSv/h";
	uint8_t sv_factor = 15; // TODO: checkme!
	uint8_t i, last=0;
	
	txtbuf[15]=0;
	
	for(i=0; i<60; i++)
		sample_buf[i]=0;
	
	init();

	TCCR0 = (1<<CS02) | (1<<CS00);
	TIMSK = (1<<TOIE0);
	
	TCCR1A = 0;
	TCCR1B = (1<<CS12) | (1<<CS11) | (1<<CS10);
	
	sei();
	
	while(1) {
		uint8_t new;
		
		cli();
		new = update;
		sei();
		
		if(new != last) {
			uint16_t sec10_copy, sec60_copy;

			last = new;
			
			cli();
			sec10_copy = sec10_count;
			sec60_copy = sec60_count;
			sei();
			
			if(rt >= 10) {
				u16_to_dec(sec10_copy, txtbuf, 1);

				if(rt >= 60) {
					uint8_t pt_pos = 1;
					
					txtbuf[15]=' '; // enable 2nd + 3rd lines
					
					u16_to_dec(sec60_copy, txtbuf+16+1, 0);

					if(sec60_copy < 6554) {
						pt_pos = 2;
						sec60_copy *= 10;
					}

					u16_to_dec(sec60_copy / sv_factor, txtbuf+(16*2), pt_pos);				
				}
				dogm_cursor(0, 0);
				dogm_print(txtbuf);
			} // rt >= 10
		} // new != last
	} // while (1)
	return 0;
}
