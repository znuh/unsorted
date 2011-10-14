all:
	avr-gcc -Wall -DF_CPU=8000000 -o servo.elf -mmcu=atmega8 -Os servo.c
	avr-size servo.elf
	avr-objcopy -O ihex -R .eeprom servo.elf servo.hex

load:
	avrdude -cstk500v2 -P /dev/ttyUSB0 -p m8 -U flash:w:servo.hex -U lfuse:w:0x64:m -U hfuse:w:0xd9:m
	
