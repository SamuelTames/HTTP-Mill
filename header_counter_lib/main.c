#include <stdio.h>
#include "header_counter_header_code.h"

int main (int argc, char *argv[])
{
  if (argc != 2) {
    printf ("Usage %s <abbreviation>\n", argv[0]);
    return 2;
  }

  char * abbr = argv[1];

  HeaderCounter * header_counter = HdrCtr_new();

  HdrCtr_valid_header(header_counter, abbr);
  HdrCtr_increment(header_counter, abbr);
  HdrCtr_print(stdout, header_counter);

  return 0;
}
 
