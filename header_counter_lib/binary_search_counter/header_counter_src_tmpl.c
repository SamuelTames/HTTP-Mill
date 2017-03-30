#include <string.h> // strcmp
#include <stdlib.h> // malloc
#include <stdio.h> // perror: ENOMEM possibly set from malloc
#include <stdbool.h> // bools
#include <assert.h> // assert

#include "../header_counter_header_code.h"


static Header headers[$NK] = {
#include "../headers.dat.h"
};

static int
find_index(HeaderCounter const * header_counter, char const * header)
{
    assert(header_counter != NULL);

    Header const * header_array = header_counter->headers;
    int index = -1;
    int low = 0;
    int high = $NK - 1;
    int ordering = 0;
    int mid = 0;

    while (low <= high) {
        mid = low + (high - low) / 2;
        ordering = strcmp(header, header_array[mid].header);

        if (ordering < 0) {
            high = mid - 1;
        } else if (ordering == 0) {
            index = mid;
            break;
        } else if (ordering > 0) {
            low = mid + 1;
        }
    }

    return index;
}

bool
HdrCtr_valid_header(HeaderCounter const * header_counter, char const * header)
{
    assert(header_counter != NULL);
    assert(header != NULL);

    int index = find_index(header_counter, header);

    return index >= 0;
}

bool
HdrCtr_increment(HeaderCounter * header_counter, char const * header)
{
    assert(header_counter != NULL);
    assert(header != NULL);

    bool result = false;
    int index = find_index(header_counter, header);

    if (index >= 0) {
        header_counter->headers[index].count += 1;
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
