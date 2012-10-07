all:
	avr-gcc -Wall -o ctl.elf -mmcu=atmega8 -Os ctl.c
	avr-objcopy -O ihex -R .eeprom ctl.elf ctl.hex
	avr-size ctl.elf

load:
	avrdude -cstk500v2 -P /dev/ttyUSB0 -p m8 -U flash:w:ctl.hex -U lfuse:w:0x64:m -U hfuse:w:0xd9:m

