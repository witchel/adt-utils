/* Implement an open hash table */
#ifndef _OPEN_HASH64_H
#define _OPEN_HASH64_H
#include <sys/types.h>

struct oht_pair {
   /* NB: Zero key is illegal */
   const u_int64_t key;
   u_int64_t data;
};
struct oht;

/* NB: ninitialpairs is rounded up to a power of 2 */
struct oht* oht_init(const char* name, u_int64_t ninitialpairs, 
      u_int64_t (*hash)(u_int64_t));
void oht_fini(struct oht* oht);
// Just lookup
struct oht_pair* oht_lookup(struct oht *oht, u_int64_t key);
// Will allocate space for entry
struct oht_pair* oht_create(struct oht *oht, u_int64_t key, int *new);
// An open hash table cannot support delete, because key a hashes to bucket A,
// Then key b also hashes to bucket A, so we rehash and put it in B.
// Erase key a, and it looks like b is missing because we find a zero key.
// int oht_erase(struct oht *oht, u_int64_t key);
// Allows users to maintain auxilliary data based on unique index
u_int64_t oht_idx(struct oht* oht, struct oht_pair*);
// Callback for each nonzero key in table 
void oht_iter_nonzero(const struct oht *oht, 
                   void (*proc)(void*, struct oht_pair*),/* Callback for each pair */
                   void *arg);
void oht_log_stats(const struct oht*);


#endif /* _OPEN_HASH64_H */
