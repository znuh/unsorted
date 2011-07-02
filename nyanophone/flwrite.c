#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU		8000000UL
#include <util/delay.h>
#include <stdint.h>

void uart_init(void) {
	UCSRC = ( 1<<URSEL) | (1<<UCSZ1) | (1<<UCSZ0);
	UBRRH = 0;
	UBRRL = 12;
	UCSRB = (1<<RXEN) | (1<<TXEN);
}

void spi_init(void) {
	SPCR = (1<<SPE) | (1<<MSTR);
}

static inline uint8_t spi_xfer(uint8_t d) {
	SPDR = d;
	while(!(SPSR & (1<<SPIF))) {}
	return SPDR;
}

static inline void spi_select(void) {
	PORTB &= ~(1<<PB0);
}

static inline void spi_deselect(void) {
	PORTB |= (1<<PB0);
}

#define CMD_WRITE_ENABLE	0x06
#define CMD_CHIP_ERASE		0x60
#define CMD_READ_STATUS		0x05

#define STATUS_BUSY			1

void write_flash(void) {
	uint8_t status;
	
	/* write enable */
	spi_select();
	spi_xfer(CMD_WRITE_ENABLE);
	spi_deselect();

	/* chip erase */
	spi_select();
	spi_xfer(CMD_CHIP_ERASE);
	spi_deselect();

	/* busywait */
	spi_select();
	spi_xfer(CMD_READ_STATUS);
	do {
		while(!(UCSRA & (1<<UDRE))) {}
		status = spi_xfer(0);
		UDR = status;
	} while(status & STATUS_BUSY);
	spi_deselect();

	
	
}

int main(void) {

	/* nSS */
	PORTB = (1<<PB0);

	/* SCK, MOSI, nSS, OC1A */
	DDRB = (1<<PB5) | (1<<PB3) | (1<<PB0) | (1<<PB1);	

	uart_init();
	spi_init();

	while(1) {
		uint8_t tmp;
		while(!(UCSRA & (1<<RXC))) {
			// TODO: read flash 
		}
		tmp = UDR;
		if(tmp == 0x23)
			write_flash();
	}
	
	return 0;
}
