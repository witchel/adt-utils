/* Implement an open hash table */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>

#include "openhash64.h"

void* debug_calloc(size_t num, size_t size){
    void*p = calloc(num, size);
    fprintf(stderr, "calloc'd %p\n",p);
    return p;
}

void* debug_malloc(size_t size){
    void*p = malloc(size);
    fprintf(stderr, "malloc'd %p\n",p);
    return p;
}

void debug_free(void*p){
    free(p);
    fprintf(stderr, "freed %p\n",p);
}

// Assume 64-byte cache lines, so each bucket has 4 pairs
#define PAIR_PER_BUCKET (4)
struct oht_bucket {
   struct oht_pair pairs[PAIR_PER_BUCKET];
};

struct oht {
   /* number of slots minus 1 to enable AND.  So number of entries
    * must be a power of 2  */
   u_int64_t nbucketmask;
   // We keep size of table a power of 2
   u_int64_t (*hash)(u_int64_t key);
   struct oht_bucket *buckets;
   const char* name;
   // Global stats for lifetime of table.
   u_int32_t expand;
   u_int64_t calls;
   u_int64_t probes;
   // These are reset on each expansion
   u_int64_t reset_probes;
   u_int64_t reset_calls;
};
static void _oht_grow(struct oht *oht);

static void* get_bucket_space(struct oht* oht) {
   void* ptr = debug_calloc(oht->nbucketmask + 1, sizeof(struct oht_bucket));
   return ptr;
}
static void free_bucket_space(struct oht* oht, void* ptr) {
    (void)oht;
   debug_free(ptr);
}


/* NB: nentryalloc is rounded up to a power of 2 */
struct oht*
oht_init(const char* name, u_int64_t ninitialpairs, u_int64_t (*hash)(u_int64_t)) {
   struct oht* oht = (struct oht*) debug_calloc(1, sizeof(*oht));
   if(!oht) return NULL;
   oht->name = name;
   oht->nbucketmask = (u_int64_t)1;
   /* Round up to nearest power of 2 */
   while(oht->nbucketmask < (ninitialpairs + PAIR_PER_BUCKET - 1)/PAIR_PER_BUCKET) {
      oht->nbucketmask <<= 1;
   }
   oht->nbucketmask--;
   oht->hash = hash;
   oht->buckets = get_bucket_space(oht);
   if(oht->buckets == NULL) {
      debug_free(oht);
      return NULL;
   }
   return oht;
}

void
oht_fini(struct oht* oht) {
   free_bucket_space(oht, oht->buckets);
   debug_free(oht);
}

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
static struct oht_pair*
find_pair(struct oht *oht, u_int64_t key, u_int64_t hbucket) {
   struct oht_bucket *ohtb = &oht->buckets[hbucket];
   // Buckets are fully associative, so first time through,
   // look for a direct match, then look for 0
   for(int i = 0; i < PAIR_PER_BUCKET; ++i) {
      if(key == ohtb->pairs[i].key) {
         return &ohtb->pairs[i];
      }
   }
   for(int i = 0; i < PAIR_PER_BUCKET; ++i) {
      if(ohtb->pairs[i].key == (u_int64_t)0) {
         return &ohtb->pairs[i];
      }
   }
   return NULL;
}
struct oht_pair*
oht_lookup(struct oht  *oht, u_int64_t key) {
   if(key == (u_int64_t)0) return NULL;
   // Keep lookup performance good
   if(oht->reset_calls > (u_int64_t)100
      && (float)oht->reset_probes/oht->reset_calls > 2.0) {
      //printf("Probes per call above 2.0, expanding\n");
      _oht_grow(oht);
   }
   u_int64_t h1bucket = oht->hash(key) & oht->nbucketmask;
   //printf("lookup %ld h1bucket %ld ", key, h1bucket);
   oht->calls++;
   oht->probes++;
   oht->reset_calls++;
   oht->reset_probes++;

   struct oht_pair *pair = find_pair(oht, key, h1bucket);
   //printf("pair %p\n", pair);
   if(pair) return pair;

   u_int64_t bucket = h1bucket;
   u_int64_t probes = (u_int64_t)0;
   do {
      // This ensures that hash2 is odd so we get full table coverage 
      // because number of buckets is a power of 2 
      // However, only nbuckets/2 slots are explored, which makes collisions more likely
      u_int64_t h2 = mult_hash(h1bucket) | 0x1;
      probes++;
      oht->probes++;
      oht->reset_probes++;
      bucket = (bucket + h2) & oht->nbucketmask;
      //printf("  lookup %ld bucket %ld ", key, bucket);
      pair = find_pair(oht, key, bucket);
      //printf("pair %p\n", pair);
      if(pair) return pair;
   } while(bucket != h1bucket);
   // Make sure we explore all buckets
   assert(probes == oht->nbucketmask + 1);
   /* Hash table is full, return NULL */
   return NULL;
}

struct oht_pair*
oht_create(struct oht *oht, u_int64_t key, int *newone) {
   if(key == (u_int64_t)0) return NULL; // No zero key
   struct oht_pair *pair = oht_lookup(oht, key);

   // Hash table can get full if there were lots of insertions without lookups
   if(pair == NULL) {
      /* Hash table is full. */
      //printf("Hash table is full, expanding\n");
      _oht_grow(oht);
      return oht_create(oht, key, newone);
   }
   if(newone) {
      if(pair->key == (u_int64_t)0) {
         *newone = 1;
      } else {
         *newone = 0;
      }
   }
   *(u_int64_t*)&pair->key = key;
   return pair;
}
/* Internal function */
/* Grow table by a factor of 2.  NB: Nentry must remain a power of 2 */
/* Unfortunately, I don't see how this can be done without having old and */
/* new resident simultaneously */
/* Make newht, insert old elments, free old ht */
/* NB: Carries over old stats */
static void
_oht_grow(struct oht *oht) {
   struct oht *newht = oht_init(oht->name,
      2 * PAIR_PER_BUCKET * (oht->nbucketmask + 1), oht->hash);
   if(newht == NULL) {
      // Run out of mePAIR_PER_BUCKETory
      //printf("No memory to grow hash table\n");
      oht_log_stats(oht);
      return;
   }
   oht->expand++;

   for(u_int64_t bucket = 0; bucket < oht->nbucketmask + 1; bucket++) {
      struct oht_bucket *ohtb = &oht->buckets[bucket];
      for(int i = 0; i < PAIR_PER_BUCKET; ++i) {
         if(ohtb->pairs[i].key != (u_int64_t)0) {
            int newone = 0;
            struct oht_pair *pair = oht_create(newht, ohtb->pairs[i].key, &newone);
            if(newone == 0) {
               //printf("Duplicate key! Bucket(%d) pair(i) key %ld data %ld\n", 
               //      bucket, i, ohtb->pairs[i].key, ohtb->pairs[i].data);
            }
            pair->data = ohtb->pairs[i].data;
         }
      }
   }
   free_bucket_space(oht, oht->buckets);
   oht->buckets = newht->buckets;
   oht->nbucketmask = newht->nbucketmask;
   oht->reset_probes = oht->reset_calls = (u_int64_t)0;
   debug_free(newht); // Not fini, we don't want to free bucket space
}

void 
oht_iter_nonzero(const struct oht *oht, 
                 void (*proc)(void*, struct oht_pair*),/* Callback for each pair */
                 void *arg)
{
   if(oht == NULL) return;
   for(u_int64_t b = (u_int64_t)0; b < oht->nbucketmask + 1; b++) {
      struct oht_bucket *ohtb = &oht->buckets[b];
      for(int i = 0; i < PAIR_PER_BUCKET; ++i) {
         if(ohtb->pairs[i].key) {
           proc(arg, &ohtb->pairs[i]);
        }
      }
   }
}

u_int64_t oht_idx(struct oht* oht, struct oht_pair* pair) {
   assert(pair >= (struct oht_pair*)oht->buckets);
   // I hope other 32-bit platforms are dying
#ifdef __i386__
   u_int64_t diff = ((u_int32_t)pair - (u_int32_t)oht->buckets)/sizeof(struct oht_pair);
#else
   u_int64_t diff = ((u_int64_t)pair - (u_int64_t)oht->buckets)/sizeof(struct oht_pair);
#endif
   return diff;
}

static u_int64_t
_oht_get_nused(const struct oht *oht) {
   u_int64_t b;
   u_int64_t nused = (u_int64_t)0;
   for(b = (u_int64_t)0; b < oht->nbucketmask + 1; b++) {
      struct oht_bucket *ohtb = &oht->buckets[b];
      for(int i = 0; i < PAIR_PER_BUCKET; ++i) {
         if(ohtb->pairs[i].key != (u_int64_t)0) {
            nused++;
         }
      }
   }
   return nused;
}
// Ugly dependence on stdio
#include <stdio.h>
void
oht_log_stats(const struct oht *oht) {
   u_int64_t nused = _oht_get_nused(oht);
   printf("Table npairs/nbuckets %llud/%llud used %llud(%3.2f%%) expand %d\n",
           (oht->nbucketmask+1) * PAIR_PER_BUCKET, oht->nbucketmask + 1,
          nused, 100.0 * nused / ((oht->nbucketmask+1) * PAIR_PER_BUCKET),
          oht->expand);
   printf("\t%llud calls %llud probes %3.2f probes/call\n",
           oht->calls, oht->probes, (double)oht->probes / oht->calls);
}

