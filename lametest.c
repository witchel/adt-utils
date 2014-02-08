#include "openhash64.h"
#include "hashes.h"
#include <stdio.h>
/*
 * Run like this:
 * $ make all
 * $ ./lametest 2> log.txt
 * $ python3 leak_detector.py log.txt  # To check for memory leaks
 */

int main() {
    struct oht* table = oht_init("Table", 256, mult_hash);
    for(unsigned int i=0; i<1000; i++){
        struct oht_pair* pair = oht_create(table, i+1, 0);
        pair->data = 99;
    }
    for(unsigned int i=0; i<10; i++){
        struct oht_pair* pair = oht_lookup(table, i+1);
        printf("%llu\n",pair->data);
    }
    oht_fini(table);
  return 0;
}
