#include "openhash64.h"
#include "hashes.h"

int main() {
    struct oht* table = oht_init("Table", 1024, mult_hash);
    
    oht_fini(table);
  return 0;
}
