CROSS_COMPILE=#mips64el-redhat-linux-
CC=$(CROSS_COMPILE)gcc
LD=$(CROSS_COMPILE)ld
CFLAGS=
LDFLAGS= 
TARGET=ttys udp

all: $(TARGET)

ttys: ttys.c Makefile
	$(CC) -o ttys -static $(CFLAGS) $(LDFLAGS) ttys.c

udp: udp.c Makefile
	$(CC) -o udp -static $(CFLAGS) $(LDFLAGS) udp.c
	
clean:
	rm $(TARGET)
