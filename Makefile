# November 24, 2015 - Sam Tames
CC=gcc -flto -O2
LDFLAGS=-pthread
CFLAGS= -Wall -Wextra -Wpedantic \
        -Wformat=2 -Wno-unused-parameter -Wshadow \
        -Wwrite-strings -Wstrict-prototypes -Wold-style-definition \
        -Wredundant-decls -Wnested-externs -Wmissing-include-dirs \
	      -Wpointer-arith -Wcast-qual -Wmissing-prototypes -Wno-missing-braces \
        -std=gnu11

EXECUTABLE=HeaderCounter
HEADER_DIR=header_counter_lib
SRCS=main.c $(HEADER_DIR)/header_counter_src_code.o
OBJS=$(SRCS:.c=.o)


all: $(EXECUTABLE)

$(EXECUTABLE): main.o
	$(CC) $(LDFLAGS) -o $(EXECUTABLE) $(OBJS)

main.o: main.c
	$(MAKE) -C $(HEADER_DIR)
	$(CC) -c $<

testdata:
	./mk_testfile.py >! testdata.txt

clean:
	rm -f $(EXECUTABLE) $(OBJS)
	$(MAKE) -C $(HEADER_DIR) clean
