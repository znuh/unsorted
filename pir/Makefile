# int  8MHz
#FUSES = -U lfuse:w:0xd4:m -U hfuse:w:0xd9:m
# ext. 16MHz
FUSES = -U lfuse:w:0xe4:m -U hfuse:w:0xdf:m

all:
	avr-gcc -Wall -o pir.elf -mmcu=attiny2313 -Os pir.c
	avr-size pir.elf
	avr-objcopy -O ihex -R .eeprom pir.elf pir.hex

load:
	avrdude -cstk500v2 -P /dev/ttyUSB0 -p t2313 -U flash:w:pir.hex $(FUSES)
	
