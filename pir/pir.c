#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#define F_CPU 8000000UL
#include <util/delay.h>

ISR(TIMER1_OVF_vect) {
}

#define TOLERANCE	4

static uint8_t idx=0;

static uint8_t pattern[]= {
	29, 29, 29, 29, 29, 29, 29,
	20,	39,
	29, 29, 29
};

static inline uint8_t match(uint8_t ref, uint8_t val) {
	if((val >= (ref-TOLERANCE)) && (val <= ref+TOLERANCE))
		return 1;
	return 0;
}

static volatile uint8_t alarm = 0;

ISR(TIMER1_CAPT_vect) {
	uint16_t val = ICR1;
	
	TCNT1 = 0;
	
	if((val < (20-TOLERANCE)) || (val > (39+TOLERANCE))) {
		idx=0;
		return;
	}
	if(match(pattern[idx], val&0xff)) {
		idx++;
		if(idx == 12) {
			alarm=1;
			idx=0;
			return;
		}
	}
	else {
		idx=0;
		return;
	}
}

int main(void)
{
	TCCR1A = 0;
	TCCR1B = (1<<ICNC1) | (1<<ICES1) | (1<<CS12);
	TCCR1C = 0;
	
	// speaker
	DDRB = (1<<PB1);
	PORTB = 0;
	
	// RF-module / LED
	DDRD = (1<<PD4);
	PORTD = (1<<PD4);
	
	TIMSK = (1<<ICIE1);

	set_sleep_mode(SLEEP_MODE_IDLE);

	sei();
	
	while (1) {
		sleep_enable();
		sleep_cpu();
		sleep_disable();
		if(alarm) {
			uint32_t i;
			TIMSK = 0;
			alarm=0;
			PORTD=0;
			for(i=0;i<10000;i++) {
				PORTB ^= (1<<PB1);
				if(i&256)
					_delay_ms(2);
				else
					_delay_ms(1);
			}
			_delay_ms(1000);
			PORTD=(1<<PD4);
			TIMSK = (1<<ICIE1);
		}
	}

	return 0;
}
