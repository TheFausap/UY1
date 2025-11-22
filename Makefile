CC = gcc
CFLAGS = -Wall -Wextra -O2

all: cpu card_maker tape_maker

cpu: cpu.c cpu.h
	$(CC) $(CFLAGS) -o cpu cpu.c

card_maker: card_maker.c cpu.h
	$(CC) $(CFLAGS) -o card_maker card_maker.c

tape_maker: tape_maker.c cpu.h
	$(CC) $(CFLAGS) -o tape_maker tape_maker.c

clean:
	rm -f cpu card_maker tape_maker *.bin *.txt output.txt

test: all
	./tape_maker lib_source.txt library.bin
	./card_maker test_source.txt deck.txt
	./cpu
	cat output.txt

.PHONY: all clean test
