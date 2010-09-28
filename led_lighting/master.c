#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU		8000000UL
#include <util/delay.h>
#include <stdint.h>

#define ETU		(F_CPU/37500)

#define MAX_LEN	30

uint16_t frame[((MAX_LEN+1)*8)+1] = {ETU*2}; // startbit
uint16_t *fptr = frame+9;
uint8_t flen=0;
uint8_t cksum=0;

ISR(TIMER1_COMPA_vect) {
	OCR1A += (*fptr);
	fptr++;
	flen--;
	if(!flen) {
		TCCR1A = 0;
		TIMSK = 0;
		fptr = frame + 9;	// prepare for new data
	}
}

uint8_t put_frame(uint8_t d) {
	uint8_t i;
	
	// no space left
	if(flen == MAX_LEN)
		return 0;
	
	cksum += d;
	
	for(i=0; i<8; i++) {
		*fptr = ETU;
		if(d&0x80)
			*fptr += (ETU/2);
		fptr++;
		d<<=1;
	}
	
	flen++;
	
	return 1;
}

void send_frame(void) {
	
	fptr = frame+1; 			// set ptr to cksum
	
	cksum = 0xff - cksum;		// insert checksum
	put_frame(cksum + 1);
	cksum = 0;
	
	fptr = frame;				// set ptr to startbit
	flen *=8; 					// bytes -> bits
	flen += 2; 				// startbit, last bit
	
	OCR1A = TCNT1 + (ETU*3); 	// silence before start
	
	TIFR = (1<<OCF1A); 		// clear flag
	TCCR1A = (1<<COM1A0); 	// enable output
	TIMSK = (1<<OCIE1A); 		// enable interrupt
}

/* TODOs:
	- doublebuffer
*/

int main(void) {
	uint16_t dimval = 0;
	
	PORTB = 0;
	DDRB = (1<<PB1);
	
	TCCR1B = (1<<CS10);
	
	// test mode enable
	PORTB = (1<<PB1);
	_delay_ms(75);
	PORTB = 0;
	
	sei();
	
	while(1) {
		uint8_t addr;
		
		for(addr=0; addr<10; addr++) {
			put_frame(addr);
			put_frame(dimval>>8);
			send_frame();
			while(TIMSK) {} // wait while send busy
		}
		dimval+= (dimval>>11) + 1;
		//dimval+=0x100;
		
		//_delay_us(127 - (dimval>>9));
		//_delay_ms(1000);
	}
	
	return 0;
}
