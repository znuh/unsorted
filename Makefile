all:
	avra -l main.lst main.asm
	avr-gcc -Wall -o master.elf -mmcu=atmega8 -Os master.c
	avr-objcopy -O ihex -R .eeprom master.elf master.hex
	avr-size master.elf

