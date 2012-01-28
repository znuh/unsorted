#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU		8000000UL
#include <util/delay.h>
#include <stdint.h>
#include <stdlib.h>

static 	uint8_t vals[3] = {0, 0, 0};

uint8_t next_val(uint8_t curr, uint8_t dst) {
	return (curr < dst) ? (curr+1) : (curr-1);
}

void dim(uint8_t id, uint8_t dst) {

	if(id>2)
		return;
	while(vals[id] != dst) {
		vals[id] = next_val(vals[id], dst);
		switch(id) {
			case 0:
				OCR2 = vals[id];
				break;
			case 1:
				OCR1A = vals[id];
				break;
			default:
				OCR1B = vals[id];
		}
		_delay_ms(10);
	}
}

int main(void) {

	// OC2, OC1B, OC1A
	PORTB = 0;
	DDRB = (1<<PB3)|(1<<PB2)|(1<<PB1);

	// Fast PWM: WGM21=1, WGM20=1
	// set bottom, clear on match: COM21=1, COM20=0
	// ((8MHz/256)/8) = 3.9kHz: CS21 = 1
	OCR2 = 0;
	TCCR2 = (1<<WGM21) | (1<<WGM20) | (1<<COM21) | (1<<CS20);

	// Fast PWM, 8-bit: WGM10=1
	// set bottom, clear on match: COM1x1=1, COM1x0=0
	// ((8MHz/256)/8) = 3.9kHz: CS11 = 1
	OCR1A = 0;
	OCR1B = 0;
	TCCR1A = (1<<WGM10) | (1<<COM1A1) | (1<<COM1B1);
	TCCR1B = (1<<CS10);

	while(1) {
		dim(rand()&3,rand()&0xff);
	}

	return 0;
}
