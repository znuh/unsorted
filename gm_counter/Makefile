all:
	avr-gcc -Wall -o main.elf -mmcu=atmega32 -Os main.c
	avr-size main.elf
	avr-objcopy -O ihex -R .eeprom main.elf main.hex

load:
	avrdude -cstk500v2 -P /dev/ttyS0 -p m32 -U flash:w:main.hex -U lfuse:w:0xe4:m -U hfuse:w:0xd9:m


