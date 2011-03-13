#define F_CPU	8000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdio.h>

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

static volatile uint16_t buf[60];
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
		
		sec10_count -= buf[sec10_idx];
		sec60_count -= buf[sec60_idx];
		
		sec10_count += delta_cnt;
		sec60_count += delta_cnt;
		
		buf[sec60_idx++] = delta_cnt;
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

void main(void) {
	char txtbuf[16];
	uint8_t i, last=0;
	
	for(i=0; i<60; i++)
		buf[i]=0;
	
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
				dogm_cursor(0, 0);
				sprintf(txtbuf,"%3d C/10s",sec10_count);
				dogm_print(txtbuf);
			}

			if(rt >= 60) {
				dogm_cursor(1,0);
				sprintf(txtbuf,"%3d C/60s",sec60_count);
				dogm_print(txtbuf);			
			}
		}
	}
}