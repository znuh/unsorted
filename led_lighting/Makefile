all:
	avra -l main.lst main.asm
	avr-gcc -Wall -o master.elf -mmcu=atmega8 -Os master.c
	avr-objcopy -O ihex -R .eeprom master.elf master.hex
	avr-size master.elf

load_master:
	avrdude -cstk500v2 -P /dev/ttyUSB0 -p m8 -U flash:w:master.hex -U lfuse:w:0x64:m -U hfuse:w:0xd9:m

load_slave:
	avrdude -cstk500v2 -P /dev/ttyUSB2 -p t13 -U eeprom:w:13:m -U flash:w:main.hex -U lfuse:w:0x79:m -U hfuse:w:0xfb:m

