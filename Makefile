CFLAGS=-Wall -g
sources = ping.c checksum.c

all: server client
server: server.o $(sources:.c=.o)
client: client.o $(sources:.c=.o)

# deps:
allsrc = $(wildcard *.c)
include $(allsrc:.c=.d)
%.d: %.c
	@set -e; rm -f $@; \
	$(CC) -M $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$
