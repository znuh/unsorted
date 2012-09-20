#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU		9216000UL
#include <util/delay.h>
#include <stdint.h>

static uint8_t bitcnt=0;
static uint8_t byte;

static uint16_t etu;

// TODOs: F_CPU fast enough? TX ringbuffer

ISR(TIMER1_COMPA_vect) {
	TCNT1 = 0;
	byte>>=1;
	byte|=0x80;
	if(bitcnt == 8) {
		TIMSK &= ~(1<<OCIE1A);
		UDR = byte;
		bitcnt = 0;
		return;
	}
	OCR1A = etu;
	bitcnt++;
}

ISR(TIMER1_CAPT_vect) {
	uint16_t val;
	
	TCNT1 = 0;
	val = ICR1;
	
	if(TCCR1B & (1<<ICES1)) {
		// rising edge - add a zero
		byte>>=1;
		if(bitcnt == 8) {
			UDR = byte;
			bitcnt = 0;
			return;
		}
		bitcnt++;
	}
	else {
		// falling edge
		// setup COMPA - val is 3/16 of a bit
		etu = (val*16)/3;
		OCR1A = etu + etu/2;
		TIFR = (1<<OCF1A);
		TIMSK = (1<<OCIE1A);
	}
	TCCR1B ^= (1<<ICES1);
}

int main(void) {
	
	UBRRH=0;
	UBRRL = 4; // 115.2 kBaud
	
	UCSRB = (1<<TXEN);
	UCSRC = (1<<URSEL)|(3<<UCSZ0); // 8 bit, 1 stopbit
	
	//UDR = 0; // test
	
	TCCR1A = 0;
	TCCR1B = (1<<ICNC1)|(1<<ICES1)|(1<<CS10);
	TIMSK = (1<<TICIE1);
	
	sei();
	
	while(1) {}
	
	return 0;
}
