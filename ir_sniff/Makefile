all:
	avr-gcc -Wall -o ir.elf -mmcu=atmega8 -Os ir.c
	avr-size ir.elf
	avr-objcopy -O ihex -R .eeprom ir.elf ir.hex

load:
	avrdude -cstk500v2 -P /dev/ttyUSB0 -p m8 -U flash:w:ir.hex -U lfuse:w:0xe0:m -U hfuse:w:0xd9:m

clean:
	rm -f *.elf *.hex

