#define F_CPU		1000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/eeprom.h>

#include <util/delay.h>

#include <stdint.h>

volatile uint32_t 		cnt		=0;
volatile uint32_t 		mark_cnt	=0;

volatile uint8_t 		events 	= 0;

#define EVT_TIMER		1
#define EVT_CNTR		2
#define EVT_MARK		4

#define EE_MEM		((const uint8_t *)0)
#define EE_SIZE		128

/*
	f_T1ovf = 1MHz/(65536*1024) = 0.014... Hz
	-> ~53.64 times/hr 
	=> eeprom written ~every 66 secs of operational time
	
	eeprom life = 100k writes, w/ wear levelling x25 = 2.5Mwrites
	* 66 secs = 165Msecs lifetime = ~5 years
	* EE_WDIV = ~20 years lifetime for eeprom
	(written every 66*4 = 264 secs = 4:24 min:sec)
*/
#define EE_WRDIV		4

const uint8_t 			*ee_ptr 	= EE_MEM;

ISR(INT0_vect) {
	if(events & EVT_MARK)
		return;
	
	mark_cnt = cnt;
	events |= EVT_MARK;
}

ISR(TIMER0_OVF_vect) {
	cnt++;
	events |= EVT_CNTR;
}

ISR(TIMER1_OVF_vect) {
	events |= EVT_TIMER;
}

uint8_t nibble2hex(uint8_t n) {
	return (n + ((n < 10) ? ('0') : ('a' - 10)));
}

void u32tostr(uint8_t *buf, uint32_t n) {
	uint8_t *ptr = buf+8;
	
	do {
		--ptr;
		*ptr = nibble2hex(n&0xf);
		n>>=4;
	}while(buf != ptr);
}

void cksum(uint8_t *buf) {
	uint8_t sum=0;
	
	for(;*buf;buf++)
		sum += *buf;
	
	*buf++ = nibble2hex(sum>>4);
	*buf = nibble2hex(sum&0xf);
}

void uart_send(char *buf) {
	for(;*buf;buf++) {
		loop_until_bit_is_set( UCSRA, UDRE );
		UDR = *buf;
	}
}

void write_eeprom(uint32_t val) {
	// TODO: write to ee_ptr, inc ee_ptr
}

void load_eeprom(void) {
	const uint8_t *ptr = EE_MEM;
	uint32_t val=0;
	
	do {
		uint8_t ckval=0x5A;
		uint8_t i=4;
		
		while(1) {
			uint8_t dbyte = ~(eeprom_read_byte(ptr++));
			ckval ^= dbyte;
			
			if(!(i--))
				break;
			
			val <<= 8;
			val |= dbyte;
			
		}
		
		if((!ckval) && (val > cnt)) {
			cnt = val;
			ee_ptr = ptr;
		}
		
	} while(ptr < (EE_MEM + (EE_SIZE/5)));
	
	// handle ptr rollover
	if(ee_ptr >= (EE_MEM + (EE_SIZE/5)))
		ee_ptr = EE_MEM;
}

int main(void) {
	char buf[30];
	uint8_t ee_timer = 0;
	
	PORTB = (1<<PB7);
	DDRB = (1<<PB7);
	
	UBRRH=0;
	UBRRL = 12; // 9,6 kBaud
	
	UCSRA = (1<<U2X);
	UCSRB = (1<<TXEN);
	UCSRC = (3<<UCSZ0); // 8 bit, 1 stopbit
	
	TCCR0A = (1<<WGM01)|(1<<WGM00);
	TCCR0B = (1<<WGM02)|(7<<CS00);
	OCR0A = 49;
	
	TCCR1A = 0;
	TCCR1B = (5<<CS10);
	TCCR1C = 0;
	
	TIMSK = (1<<TOIE1) | (1<<TOIE0);
	
	MCUCR = (1<<ISC01);
	GIMSK = (1<<INT0);
	
	set_sleep_mode(SLEEP_MODE_IDLE);
	
	buf[0]='*';
	buf[9] = buf[18] = ',';
	buf[21]='#';
	buf[22]='\r';
	buf[23]='\n';
	buf[24]=0;
	
	load_eeprom();
	
	while(1) {
		
		cli();
		
		if(events & EVT_TIMER) {
			uint32_t cnt_copy 	= cnt;
			uint32_t mark_copy 	= mark_cnt;
			uint8_t evts_copy 	= events;
			
			events = 0;
			
			sei();
			
			PORTB &= ~(1<<PB7);	// LED on
			
			u32tostr((uint8_t*)buf+1, cnt_copy);
			u32tostr((uint8_t*)buf+1+8+1, mark_copy);
			
			buf[19]=0;
			cksum((uint8_t *)buf);
			
			uart_send(buf);
			
			/* eeprom write pending? */
			if(ee_timer) {
				ee_timer--;
				
				if(!ee_timer)
					write_eeprom(cnt_copy);
			}
			else if(evts_copy & EVT_CNTR)
				ee_timer = EE_WRDIV; 	// counter changed -> schedule eeprom write
			
			PORTB |= (1<<PB7);	// LED off
		}
		else
			sei();
		
		sleep_mode();
	}
	
	return 0;
}
