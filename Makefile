CROSS_COMPILE=#mips64el-redhat-linux-
CC=$(CROSS_COMPILE)gcc
LD=$(CROSS_COMPILE)ld
DEFS= -DDEBUG=1
CFLAGSi += $(DEFS)
LDFLAGS= 
TARGET=ttys udp tcp

all: $(TARGET)

ttys: ttys.o
	$(CC) -o ttys -static $(CFLAGS) $(LDFLAGS) ttys.c

udp: udp.o
	$(CC) -o udp -static $(CFLAGS) $(LDFLAGS) udp.c
	
tcp: tcp.o
	$(CC) -o tcp -static $(CFLAGS) $(LDFLAGS) tcp.c

clean:
	rm -f $(TARGET) *.o
