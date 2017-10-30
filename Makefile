CROSS_COMPILE=
CC=$(CROSS_COMPILE)gcc
LD=$(CROSS_COMPILE)ld
CFLAGS=
LDFLAGS= 

all: ttys

ttys: ttys.c Makefile
	$(CC) -o ttys -static $(CFLAGS) $(LDFLAGS) ttys.c
	
clean:
	rm ttys
