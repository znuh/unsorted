all:
	avr-gcc -Wall -o sndtrig.elf -mmcu=attiny13 -Os sndtrig.c
	avr-size sndtrig.elf
	avr-objcopy -O ihex -R .eeprom sndtrig.elf sndtrig.hex

load:
	# fuses: internal 128kHz oscillator, no clock div, BOD at 2.7V
	avrdude -cstk500v2 -B 40 -P $(STKDEV) -p t13 -U flash:w:sndtrig.hex -U lfuse:w:0x7b:m -U hfuse:w:0xfb:m
