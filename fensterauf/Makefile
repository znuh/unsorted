all:
	avr-gcc -Wall -o sndtrig.elf -mmcu=attiny13 -Os sndtrig.c
	avr-size sndtrig.elf
	avr-objcopy -O ihex -R .eeprom sndtrig.elf sndtrig.hex

load:
	avrdude -cstk500v2 -P $(STKDEV) -p t13 -U flash:w:sndtrig.hex
