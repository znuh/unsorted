#include <avr/io.h>
#include <avr/interrupt.h>
//#define F_CPU		3686400UL
#include <util/delay.h>
#include <stdint.h>

volatile static uint8_t bitcnt=0;
volatile static uint8_t byte;

//static uint16_t etu;

ISR(TIMER1_COMPA_vect) {
	TCNT1 = 0;
	//TIMSK &= ~(1<<OCIE1A);
	byte>>=1;
	byte|=0x80;
	if(bitcnt == 8) {
		TIMSK &= ~(1<<OCIE1A);
		UDR = byte;
		bitcnt = 0;
		return;
	}
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
		// setup COMPA - TODO
		
		TIFR = (1<<OCF1A);
		TIMSK = (1<<OCIE1A);
	}
	TCCR1B ^= (1<<ICES1);
}

int main(void) {
	
	UBRRH=0;
	UBRRL = 1; // 115.2 kBaud
	
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
