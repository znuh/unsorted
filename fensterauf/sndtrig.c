
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#define MIN(a,b) ((a)<=(b)?(a):(b))

/* 
 * PB4: enable IN
 * PB2: trigger OUT
 * PB1: CLK IN
 * PB0: DATA IN
 * 
 * clock sources:
 * 4.8 MHz, 9.6 MHz
 * 128 kHz oscillator
 * CLKPR: div 1/2/4/8/16/32/64/128/256
 * DIV8 fuse
 * 
 * chosen system clock: 4.8MHz/8 = 600kHz
 *
 * watchdog timer: 128kHz /2k    /4k    /8k    /16k     /32k /64k /128k /256k /512k /1024k
 *                        16ms / 32ms / 64ms / 0.125s / 0.25s / 0.5s / 1s / 2s / 4s / 8s
 * 
 * f_clk_io (for T0): /8  /64  /256  /1024
 */

static uint8_t rxbyte = 0;
static uint8_t rxcount = 0;
static uint8_t rxbuf[5];

static volatile uint8_t co2_msb = 0;

ISR(INT0_vect) {                  /* __vector_1 */
	rxbyte<<=1;
	rxbyte |= PINB&1;

	TCNT0 = 0;

	if(rxcount >= 40)
		return;

	rxcount++;

	if(rxcount&7)
		return;

	rxbuf[(rxcount>>3)-1] = rxbyte;
	if((rxcount == 40) && (rxbuf[0] == 0x50) && (rxbyte == 0x0d)) {
		uint8_t cksum = 0x50;
		cksum += rxbuf[1];
		cksum += rxbuf[2];
		if(cksum == rxbuf[3])
			co2_msb = rxbuf[1];
	}
}

/* system clock: 4.8MHz/8   = 600  kHz  equals 1.67 us
 * f_T0 = system clock / 64 = 9.375kHz  equals  107 us
 * T0 overflow after 256     : ~36.6Hz  equals   27 ms
 */
ISR(TIM0_OVF_vect) {              /* __vector_3 */
	rxcount = 0;
}

/* systick: 1/8 Hz
 * 256 * 8s = ~34min
 */

static volatile uint8_t systick = 0;

ISR(WDT_vect) {                   /* __vector_8 */
	systick++;
}

#define TRIGGER_OUT      PB2
#define nENABLE_IN       PB4

int main(void) {
	uint8_t last_trigger = 255;

	PORTB = (1<<nENABLE_IN);          /* enable pullup */
	DDRB = (1<<TRIGGER_OUT);

	TCCR0B = (1<<CS01)|(1<<CS00);     /* prescaler: 64 */
	TIMSK0 = (1<<TOIE0);

	WDTCR = (1<<WDCE);                /* enable watchdog IRQ every 8s */
	WDTCR = (1<<WDCE) | (1<<WDTIE) | (1<<WDP3) | (1<<WDP0);

	MCUCR = (1<<ISC01)|(1<<ISC00);    /* enable INT0 on rising edge */
	GIMSK = (1<<INT0);

	set_sleep_mode(SLEEP_MODE_IDLE);

	sei();

	sleep_enable();

	while(1) {
		uint8_t next_trigger, co2_tmp, last_systick = systick;

		do {
			sleep_cpu();
		} while(systick == last_systick);

		if(!last_trigger)
			PORTB &= ~(1<<TRIGGER_OUT);

		if(last_trigger<255)
			last_trigger++;

		co2_tmp = co2_msb;
		if((PINB & (1<<nENABLE_IN)) || (co2_tmp <= 3)) {
			last_trigger = 255;
			continue;
		}

		next_trigger = co2_tmp - 4;
		next_trigger = MIN(next_trigger, 0x1f);          /* max: ~8k */
		next_trigger = 0x1f-next_trigger;
		next_trigger<<=3;

		if(last_trigger < next_trigger)
			continue;

		last_trigger = 0;
		PORTB |= (1<<TRIGGER_OUT);
	}

	return 0;
}
