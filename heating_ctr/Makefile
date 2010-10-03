all:
	avr-gcc -Wall -o heating_ctr.elf -mmcu=attiny2313 -Os heating_ctr.c
	avr-size heating_ctr.elf
	avr-objcopy -O ihex -R .eeprom heating_ctr.elf heating_ctr.hex

load:
	avrdude -cstk500v2 -P /dev/ttyUSB0 -p t2313 -U flash:w:heating_ctr.hex -U lfuse:w:0x64:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m
