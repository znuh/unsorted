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

volatile uint8_t vals[16]; // = {0x10, 0x20, 0x40, 0x80, 0xff, 0x10, 0x20, 0x40};
static uint8_t have_addr = 0;
static uint8_t addr = 0;

ISR(USART_RXC_vect) {
	uint8_t val = UDR;
	if(have_addr) {
		vals[addr] = val;
		have_addr = 0;
	}
	else if (val < 15) {
		have_addr = 1;
		addr = val;
	}
}

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

static uint8_t uart_rcv(void) {
	uint8_t d;
	while(!(UCSRA & (1<<RXC)));
	d = UDR;
	UDR = d;
	return d;
}

int main(void) {
	uint8_t i;
	
	PORTB = 0;
	DDRB = (1<<PB1);
	
	TCCR1B = (1<<CS10);
	
	// test mode enable
	PORTB = (1<<PB1);
	_delay_ms(75);
	PORTB = 0;

	UCSRB = (1<<RXCIE) | (1<<RXEN) | (1<<TXEN);
	UCSRC = (1<<URSEL) | (1<<UCSZ1) | (1<<UCSZ0);
	UBRRH = 0;
	UBRRL = 12;
	
	for(i=0;i<16;i++)
		vals[i] = i<<4;
	
	sei();
	
	while(1) {
		for(i=0;i<=15;i++) {
			put_frame(i);
			put_frame(vals[i]);
			send_frame();
			while(TIMSK) {} // wait while send busy
		}
	}
	
	return 0;
}
