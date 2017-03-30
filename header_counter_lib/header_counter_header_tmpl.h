#ifndef HEADER_COUNTER_H
#define HEADER_COUNTER_H

#include <stdio.h>
#include <stdbool.h>

#define MAX_HEADER_LENGTH $NS

typedef struct Header {
    char const * header;
    int count;
} Header;

typedef struct HeaderCounter {
    int size;
    Header headers[];
} HeaderCounter;

void HdrCtr_merge_counters(HeaderCounter * dest, HeaderCounter const * src);
void HdrCtr_print(FILE *, HeaderCounter const *);
bool HdrCtr_valid_header(HeaderCounter const *, char const * header);
bool HdrCtr_increment(HeaderCounter *, char const * header);
HeaderCounter * HdrCtr_new(void);
void HdrCtr_free(HeaderCounter *);
// #define HdrCtr_free(header_counter) do {free(header); header_counter = NULL;} while(0)

#endif // HEADER_COUNTER_H
