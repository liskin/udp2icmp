CFLAGS=-Wall -g --std=c99
sources = ping.c checksum.c

all: main
	ln -s -f main udp2icmpsrv
	ln -s -f main udp2icmpcli

main: main.o $(sources:.c=.o)

# deps:
allsrc = $(wildcard *.c)
include $(allsrc:.c=.d)
%.d: %.c
	@set -e; rm -f $@; \
	$(CC) -M $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

clean:
	git clean -f -X
