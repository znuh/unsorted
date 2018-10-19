all:
	avr-gcc -g -Wall -o giessomat.elf -mmcu=atmega328p -Os giessomat.c
	avr-size giessomat.elf
	avr-objcopy -O ihex -R .eeprom giessomat.elf giessomat.hex

load:
	#stty 9600 < /dev/ttyUSB0
	#echo -n "UUUU" > /dev/ttyUSB0
	avrdude -cavrisp -P /dev/ttyUSB0 -b 57600 -p m328p -U flash:w:giessomat.hex 
	
