
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#define MIN(a,b) ((a)<=(b)?(a):(b))

/* 
 * PB1: CLK
 * PB0: DATA
 * 
 * clock sources:
 * 4.8 MHz, 9.6 MHz
 * 128 kHz oscillator
 * CLKPR: div 1/2/4/8/16/32/64/128/256
 * DIV8 fuse
 * 
 * watchdog timer: 128kHz /2k    /4k    /8k    /16k     /32k /64k /128k /256k /512k /1024k
 *                        16ms / 32ms / 64ms / 0.125s / 0.25s / 0.5s / 1s / 2s / 4s / 8s
 * 
 * f_clk_io (for T0): /8  /64  /256  /1024
 */

static uint8_t rxbyte = 0;
static uint8_t rxcount = 0;
static uint8_t rxbuf[5];

static uint8_t co2_msb = 0;

ISR(INT0_vect) {
	rxbyte<<=1;
	rxbyte |= PINB & (1<<PB0);

	TCNT0 = 0;

	if(rxcount >= 40)
		return;

	rxcount++;

	if(rxcount&7)
		return;

	rxbuf[rxcount>>3] = rxbyte;
	if((rxcount == 40) && (rxbuf[0] == 0x50) && (rxbuf[4] == 0x0d) && (rxbuf[1]+rxbuf[2]+0x50 == rxbuf[3]))
		co2_msb = rxbuf[1];
}

/* 128kHz                 equals 7.8125 usec
 * 128kHz/256 = 500Hz     equals 2ms
 * x8 overflow after 16ms
 */
ISR(TIM0_OVF_vect) {
	rxcount = 0;
}

/* systick: 1/8 Hz
 * 256 * 8s = ~34min
 */

static uint8_t last_trigger = 0;

ISR(WDT_vect) {
	uint8_t next_trigger;

	if(!last_trigger)
		PORTB = 0;

	if(last_trigger<255)
		last_trigger++;

	if(co2_msb < 4)                         /* do nothing while <1k */
		return;

	next_trigger = co2_msb - 4;
	next_trigger = MIN(next_trigger, 0x1f); /* max: ~8k */
	next_trigger = 0x1f-next_trigger;
	next_trigger<<=3;

	if(next_trigger < last_trigger)
		return;

	last_trigger = 0;
	PORTB |= (1<<PB2);
}

int main(void) {

	DDRB = (1<<PB2);

	TCCR0B = (1<<CS01);
	TIMSK0 = (1<<TOIE0);

	WDTCR = (1<<WDCE);
	WDTCR = (1<<WDCE) | (1<<WDTIE) | (1<<WDP3) | (1<<WDP0);

	MCUCR = (1<<ISC01)|(1<<ISC00);
	GIMSK = (1<<INT0);

	set_sleep_mode(SLEEP_MODE_IDLE);

	sei();

	sleep_enable();

	while(1) {
		sleep_cpu();
	}

	return 0;
}
