CFLAGS=-Wall -g
sources = main.c ping.c checksum.c

main: $(sources:.c=.o)

# deps:
include $(sources:.c=.d)
%.d: %.c
	@set -e; rm -f $@; \
	$(CC) -M $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$
