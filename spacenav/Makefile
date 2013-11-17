
all:
	gcc -O2 -fPIC -Wall -c spacenav.c
	gcc -O2 -fPIC -Wall -I/usr/include/lua5.2 -c spacenav_lua.c
	gcc -Wall -DTEST_LIB -o spacenav spacenav.c -lpthread
	gcc -O -shared -fPIC spacenav.o spacenav_lua.o -o spacenav.so -llua5.2

clean:
	rm -f *.o spacenav *~

