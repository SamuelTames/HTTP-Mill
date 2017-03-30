#include <stdlib.h> //malloc
#include <stdio.h> //perror: ENOMEM possibly set from malloc
#include <stdbool.h> // bools
#include <assert.h> // assert
#include <string.h> // strcmp

#include "../header_counter_header_code.h"


static Header const headers[$NK] = {
#include "../headers.dat.h"
};

static int T1[] = { $S1 };
static int T2[] = { $S2 };
static int G[] = { $G };

static int
hash_g(char const * key, int const * T)
{
    assert(key != NULL);

    int i = 0;
    int sum = 0;

    for (i = 0; key[i] != '\0'; i += 1) {
        sum += T[i] * key[i];
        //    sum %= $NG;
    }
    return G[sum % $NG];
}

static int
perfect_hash(char const *key)
{
    assert(key != NULL);

    return (hash_g (key, T1) + hash_g (key, T2)) % $NG;
}

bool
HdrCtr_valid_header(HeaderCounter const * header_counter, char const * header)
{
    assert(header_counter != NULL);
    assert(header != NULL);

    bool result = false;
    int hash_value = perfect_hash(header);

    if (hash_value < $NK && strcmp(header, header_counter->headers[hash_value].header) == 0) {
        result = true;
    }

    return result;
}

bool
HdrCtr_increment(HeaderCounter * header_counter, char const * header)
{
    assert(header_counter != NULL);
    assert(header != NULL);

    bool result = false;
    int hash_value = perfect_hash(header);

    if (hash_value < $NK && strcmp(header, header_counter->headers[hash_value].header) == 0) {
        header_counter->headers[hash_value].count += 1;
        result = true;
    }

    return result;
}

HeaderCounter *
HdrCtr_new(void)
{
    HeaderCounter * header_counter = malloc(sizeof(HeaderCounter) + sizeof(headers));
    if (header_counter == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    header_counter->size = sizeof(headers) / sizeof(headers[0]);
    memcpy(header_counter->headers, headers, sizeof(headers));

    return header_counter;
}

void
HdrCtr_free(HeaderCounter * header_counter)
{
    free(header_counter);
}


void
HdrCtr_print(FILE * output, HeaderCounter const * hc)
{
    assert(output != NULL);
    assert(hc != NULL);

    int i = 0;
    for (i = 0; i < $NK; i += 1) {
        fprintf(output, "%s %d\n", hc->headers[i].header, hc->headers[i].count);
        fflush(output);
    }
}

// an atomic add could be used here to avoid outside locking.
void
HdrCtr_merge_counters(HeaderCounter * dest, HeaderCounter const * source)
{
    assert(dest != NULL);
    assert(source != NULL);

    int i = 0;
    for (i = 0; i < $NK; i += 1) {
        dest->headers[i].count += source->headers[i].count;
    }
}
