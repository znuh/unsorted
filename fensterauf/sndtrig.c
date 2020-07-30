#define F_CPU		1000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#include <util/delay.h>

#include <stdint.h>

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

static uint8_t systick = 0;

ISR(WDT_vect) {
	systick++;
}

int main(void) {

	TCCR0B = (1<<CS01);
	TIMSK0 = (1<<TOIE0);

/*	
	MCUCR = (1<<ISC01);
	GIMSK = (1<<INT0);
*/
	
	set_sleep_mode(SLEEP_MODE_IDLE);
	
	sei();
		
	while(1) {
		sleep_mode();
	}
	
	return 0;
}
