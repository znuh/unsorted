#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU		8000000UL
#include <util/delay.h>
#include <stdint.h>

static void uart_init(void) {
	UCSRC = ( 1<<URSEL) | (1<<UCSZ1) | (1<<UCSZ0);
	UBRRH = 0;
	UBRRL = 12;
	UCSRB = (1<<RXEN) | (1<<TXEN);
}

static inline void uart_busywait(void) {
	while(!(UCSRA & (1<<UDRE))) {}
}

static uint8_t uart_rcv(void) {
	while(!(UCSRA & (1<<RXC))) {}
	return UDR;
}

static void uart_snd(uint8_t d) {
	uart_busywait();
	UDR = d;
}

static void spi_init(void) {
	SPCR = (1<<SPE) | (1<<MSTR);
}

static uint8_t spi_xfer(uint8_t d) {
	SPDR = d;
	while (!(SPSR & (1<<SPIF)));
	return SPDR;
}

static inline void spi_select(void) {
	PORTB &= ~(1<<PB2);
}

static inline void spi_deselect(void) {
	PORTB |= (1<<PB2);
}

#define CMD_WRITE_ENABLE	0x06
#define CMD_WRITE_DISABLE	0x04
#define CMD_CHIP_ERASE		0x60
#define CMD_READ_STATUS		0x05
#define CMD_READ_DEVID		0x9f
#define CMD_WRITE_SEQ		0xaf
#define CMD_READ_ARRAY		0x0b
#define CMD_SECTOR_UNPROTECT	0x39	

#define STATUS_BUSY			1

static uint8_t read_status(void) {
	uint8_t ret;

	spi_select();
	spi_xfer(CMD_READ_STATUS);
	ret = spi_xfer(0);
	spi_deselect();
	
	return ret;
}

static uint8_t wait_flash(uint8_t send_uart) {
	uint8_t status;
	
	/* busywait */
	spi_select();
	spi_xfer(CMD_READ_STATUS);
	do {
		status = spi_xfer(0);
		if(send_uart)
			uart_snd(status);
	} while(status & STATUS_BUSY);
	spi_deselect();	

	return status;
}

static void write_flash(void) {
	uint16_t sector;
	
	/* unprotect sectors */
	for(sector=0; sector<0x800; sector+=0x20) {
		
		/* write enable */
		spi_select();
		spi_xfer(CMD_WRITE_ENABLE);
		spi_deselect();

		/* sector unprotect */
		spi_select();
		spi_xfer(CMD_SECTOR_UNPROTECT);
		spi_xfer(sector>>8);
		spi_xfer(sector&0xff);
		spi_xfer(0);
		spi_deselect();
	}

	/* write enable */
	spi_select();
	spi_xfer(CMD_WRITE_ENABLE);
	spi_deselect();

	/* chip erase */
	spi_select();
	spi_xfer(CMD_CHIP_ERASE);
	spi_deselect();

	wait_flash(1);

	/* write enable */
	spi_select();
	spi_xfer(CMD_WRITE_ENABLE);
	spi_deselect();

	/* start sequential write */
	spi_select();
	spi_xfer(CMD_WRITE_SEQ);
	spi_xfer(0);
	spi_xfer(0);
	spi_xfer(0);

	while(1) {
		uint8_t status, data, cmd = uart_rcv();

		/* continue write? */
		if(cmd != 'A')
			break;
		
		data = uart_rcv();
		spi_xfer(data);
		spi_deselect();

		/* busywait */
		status = wait_flash(0);
		uart_snd(status);

		spi_select();
		spi_xfer(CMD_WRITE_SEQ);
	}
	spi_deselect();
	
	/* write disable */
	spi_select();
	spi_xfer(CMD_WRITE_DISABLE);
	spi_deselect();
	
}

#define FLASH_SIZE	524288UL

static void read_flash(void) {
	uint32_t i;
		
	spi_select();
	spi_xfer(CMD_READ_ARRAY);
	spi_xfer(0);
	spi_xfer(0);
	spi_xfer(0);
	spi_xfer(0);

	for(i=0; (i<FLASH_SIZE) && (!(UCSRA & (1<<RXC))); i++) {
		uint8_t data = spi_xfer(0);
		uart_snd(data);
	}
	spi_deselect();
}

int main(void) {

	/* nSS */
	PORTB = (1<<PB2);

	/* SCK, MOSI, nSS, OC1A */
	DDRB = (1<<PB5) | (1<<PB3) | (1<<PB2) | (1<<PB1);	

	uart_init();
	spi_init();

	while(1) {
		uint8_t cmd = uart_rcv();
		if(cmd == 'A')
			write_flash();
		else if(cmd == 'B')
			read_flash();
		else if(cmd == 'C')
			uart_snd(read_status());
		else if(cmd == 'D') {
			uint8_t i;
			spi_select();
			spi_xfer(CMD_READ_DEVID);
			for(i=0;i<4;i++)
				uart_snd(spi_xfer(0));
			spi_deselect();
		}
		else
			uart_snd(cmd);
	}
	
	return 0;
}
