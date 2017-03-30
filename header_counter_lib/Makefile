# November 24, 2015 - Sam Tames
# I suspect this could be done a bit better.
CC=gcc -flto -O2
CFLAGS= -Wall -Wextra -Wpedantic \
        -Wformat=2 -Wno-unused-parameter -Wshadow \
        -Wwrite-strings -Wstrict-prototypes -Wold-style-definition \
        -Wredundant-decls -Wnested-externs -Wmissing-include-dirs \
	      -Wpointer-arith -Wcast-qual -Wmissing-prototypes -Wno-missing-braces \
        -std=gnu11

HEADER_DIR=hash_counter
EXECUTABLE=$(HEADER_DIR)_test


header_counter: $(HEADER_DIR)/header_counter_src_code.o

$(HEADER_DIR)/header_counter_src_code.o: $(HEADER_DIR)/header_counter_src_code.c header_counter_header_code.h
	$(CC) -c $<

$(HEADER_DIR)/header_counter_src_code.c: headers.dat.h $(HEADER_DIR)/header_counter_src_tmpl.c
	./perfect_hash.py --keycol 1 --hft 2 headers.dat $(HEADER_DIR)/header_counter_src_tmpl.c

header_counter_header_code.h: headers.dat.h header_counter_header_tmpl.h
	./perfect_hash.py --keycol 1 --hft 2 headers.dat header_counter_header_tmpl.h

headers.dat.h: headers.dat
	./mk_header.py > $@

check: main.o $(HEADER_DIR)/header_counter_src_code.o
	$(CC) -o $(EXECUTABLE) main.o header_counter_src_code.o

main.o: main.c header_counter_header_code.h
	$(CC) -c $<

clean:
	rm -f $(EXECUTABLE) main.o headers.dat.h $(HEADER_DIR)/header_counter_src_code.* header_counter_src_code.o header_counter_header_code.h
