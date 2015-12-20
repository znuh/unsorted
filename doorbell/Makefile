all:
	avra -l main.lst main.asm

load:
	avrdude -cstk500v2 -P /dev/ttyUSB0 -p t13 -U flash:w:main.hex -U lfuse:w:0x67:m -U hfuse:w:0xfb:m

