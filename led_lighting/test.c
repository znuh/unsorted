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

	OCR1B = ((uint16_t)0x100 - (uint16_t)TCNT0) + OCR1A;
	while(1) {}	
	return 0;
}
