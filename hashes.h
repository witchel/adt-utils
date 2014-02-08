/* Implement an open hash table */
#ifndef _HASHES_H
#define _HASHES_H
#include <sys/types.h>

// A simple and fast multiplicative hash function for 64-bits that uses a golden ratio
// multiplier because Knuth says so.
static u_int64_t
mult_hash(u_int64_t _a){
    unsigned char *a = (unsigned char*)&_a;
    u_int64_t h = _a;
    h = 0x9e3779b97f4a7c13ULL * h + a[0];
    h = 0x9e3779b97f4a7c13ULL * h + a[1];
    h = 0x9e3779b97f4a7c13ULL * h + a[2];
    h = 0x9e3779b97f4a7c13ULL * h + a[3];
    h = 0x9e3779b97f4a7c13ULL * h + a[4];
    h = 0x9e3779b97f4a7c13ULL * h + a[5];
    h = 0x9e3779b97f4a7c13ULL * h + a[6];
    h = 0x9e3779b97f4a7c13ULL * h + a[7];
    return h;
}


#endif /* _HASHES_H */
