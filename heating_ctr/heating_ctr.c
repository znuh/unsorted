#define F_CPU		1000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/eeprom.h>

#include <util/delay.h>

#include <stdint.h>

volatile uint32_t cnt=0;
volatile uint32_t mark_cnt=0;

volatile uint8_t send_request = 1;
volatile uint8_t mark_request = 0;

uint8_t ee_mem[128] EEMEM;
const uint8_t *ee_ptr = ee_mem;
uint32_t ee_cnt;

ISR(INT0_vect) {
	if(mark_request)
		return;
	
	mark_cnt = cnt;
	mark_request = 1;
}

ISR(TIMER0_OVF_vect) {
	cnt++;
}

ISR(TIMER1_OVF_vect) {
	send_request = 1;
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

void write_eeprom(void) {
	// TODO: write to ee_ptr, inc ee_ptr
}

void load_eeprom(void) {
	const uint8_t *ptr = ee_mem;
	uint32_t val=0;
	
	do {
		uint8_t cksum=0x5A;
		uint8_t i=4;
		
		while(1) {
			uint8_t dbyte = ~(eeprom_read_byte(ptr++));
			cksum ^= dbyte;
			
			if(!(i--))
				break;
			
			val <<= 8;
			val |= dbyte;
			
		}
		
		if((!cksum) && (val > cnt)) {
			cnt = val;
			ee_ptr = ptr;
		}
		
	} while(ptr < (ee_mem + (128/5)));
	
	// handle ptr rollover
	if(ee_ptr >= (ee_mem + (128/5)))
		ee_ptr = ee_mem;
	
	// save eeprom val
	ee_cnt = cnt;
}

int main(void) {
	char buf[30];
		
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
	
	//load_eeprom();
	
	while(1) {
		
		cli();
				
		if(send_request) {
			uint32_t cnt_copy = cnt;
			uint32_t mark_copy = mark_cnt;
			
			send_request = 0;
			mark_request = 0;
			
			sei();
			
			PORTB &= ~(1<<PB7);	// LED on
			
			u32tostr((uint8_t*)buf+1, cnt_copy);
			u32tostr((uint8_t*)buf+1+8+1, mark_copy);
			
			buf[19]=0;
			cksum((uint8_t *)buf);
			
			uart_send(buf);
			
			if(ee_cnt != cnt_copy) { // TODO: additional div
				ee_cnt = cnt_copy;
				//write_eeprom();
			}
			
			PORTB |= (1<<PB7);	// LED off
		}
		else
			sei();
		
		sleep_mode();
	}
	
	return 0;
}
