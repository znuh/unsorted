
all:
	gcc -O2 -fPIC -Wall -I/usr/include/lua5.2 -c seccomp_lua.c
	gcc -O -shared -fPIC seccomp_lua.o -o seccomp.so -llua5.2

clean:
	rm -f *.o *.so *~
