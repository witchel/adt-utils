#include "openhash64.h"
#include "hashes.h"

/*
 * Run like this:
 * $ make all
 * $ ./lametest 2> log.txt
 * $ python3 leak_detector.py log.txt  # To check for memory leaks
 */

int main() {
    struct oht* table = oht_init("Table", 1024, mult_hash);
    
    oht_fini(table);
  return 0;
}
