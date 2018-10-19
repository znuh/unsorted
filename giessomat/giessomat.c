#define F_CPU		16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>

#include <util/delay.h>

#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>

#define U16_INC(x)		{ if((x)<65535) { x++; } }

#ifndef BOOT_SECTION_START
#define BOOT_SECTION_START	0x3C00
#endif

//typedef void (*bldr_t)(void) __attribute__((noreturn));
//const bldr_t jump_to_bootloader = (bldr_t)(BOOT_SECTION_START/2);

static inline void uart_send(const char *buf) {
	for(;*buf;buf++) {
		loop_until_bit_is_set( UCSR0A, UDRE0 );
		UDR0 = *buf;
	}
	/*
	txptr = buf;
	UCSR0B |= 1<<UDRIE0;
	*/
}

int main(void) {

	UCSR0A = 0; //(1<<U2X0);
	UCSR0B = (1<<TXEN0)|(1<<RXEN0); //|(1<<RXCIE0);
	UCSR0C = (3<<UCSZ00); // 8 bit, 1 stopbit
	UBRR0H = 0;
	UBRR0L = 25; // 38.4 kBaud

	/*
	 * PB5: LED
	 */
	
	PORTB = 0;
	DDRB = (1<<PB5);

	TCCR1A = 0;
	TCCR1B = (1<<CS12) | (1<<CS11) | (1<<CS10);

	while(1) {
		char buf[16];
		uint16_t val;
		
		/* LED */
		PORTB = (1<<PB5);
		
		TCNT1 = 0;
		_delay_ms(100);
		val = TCNT1;
		sprintf(buf,"%"PRIu16"\r\n",val);
		uart_send(buf);
		
		/* LED */
		PORTB &= ~(1<<PB5);
		
		_delay_ms(1000);
	}
	
	return 0;
}
