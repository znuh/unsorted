#define F_CPU 8000000
#include <util/delay.h>
#include <avr/io.h>

int main(void)
{
    uint16_t i;
	
    DDRB = (1<<PB1)|(1<<PB2);
    
	/* to get 50Hz (20ms) we need 20000 */
	ICR1=20000;

	/* middle is 750 */
	OCR1A=1500;
	OCR1B=1500;

	
	TCCR1A=(1<<COM1A1)|(1<<COM1B1)|(1<<WGM11);
    
	/* timer runs at 1MHz (1us) */	
	TCCR1B=(1<<WGM13)|(1<<WGM12)|(1<<CS11);


	/* min (1ms) is 500 */

	/* max (2ms) is 1000 */


	while(1) {
		for(i=1000;i<2000;i+=100) {
			OCR1A = OCR1B = i;
			_delay_ms(100);
		}
		for(i=2000;i>=1000;i-=100) {
			OCR1A = OCR1B = i;
			_delay_ms(100);
		}
	}

    return 0;
}
