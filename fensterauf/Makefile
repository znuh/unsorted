all:
	avr-gcc -Wall -o sndtrig.elf -mmcu=attiny13 -Os sndtrig.c
	avr-size sndtrig.elf
	avr-objcopy -O ihex -R .eeprom sndtrig.elf sndtrig.hex

load:
	# fuses: internal 4.8MHz oscillator, div8, BOD at 2.7V
	avrdude -cstk500v2 -B 40 -P $(STKDEV) -p t13 -U flash:w:sndtrig.hex -U lfuse:w:0x69:m -U hfuse:w:0xfb:m
