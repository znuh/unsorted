all:
	avr-gcc -Wall -o flwrite.elf -mmcu=atmega8 -Os flwrite.c
	avr-objcopy -O ihex -R .eeprom flwrite.elf flwrite.hex
	avr-size flwrite.elf

load:
	avrdude -cstk500v2 -P /dev/ttyS1 -p m8 -U flash:w:flwrite.hex -U lfuse:w:0x64:m -U hfuse:w:0xd9:m
