/**
 * @file util/cuckoo-hash.h
 *
 * @date 2014
 *
 * @author Georgy Bronnikov <gogabr@yandex-team.ru>
 *
 * @copyright 2014 Yandex. All rights reserved.
 *
 * Cuckoo hash implementation.
 */

#ifdef CUCKOO_HASH_H
#  error "cuckoo-hash.h: double inclusion"
#endif
#define CUCKOO_HASH_H

/* NOTE: there is no way to store a zero value in these tables */
typedef struct cuckoo_hash_table cuckoo_hash_table_t;

/* pointer at the start of the table is public */
struct cuckoo_hash_table_base {
  void *(*lookup)(cuckoo_hash_table_t *cht, void *key);
};

typedef uint32_t cuckoo_hash_ft(void *key, void *aux);
typedef bool cuckoo_check_ft(void *key, void *val, void *aux);

/* Keys and entries are memcpy'd when inserted. */
cuckoo_hash_table_t *make_cuckoo_hash_table(int32_t key_size, int32_t entry_size,
                                            int nhashes, cuckoo_hash_ft **hashes, cuckoo_check_ft *check,
                                            void *aux);
void cuckoo_free(cuckoo_hash_table_t *cht);

void cuckoo_set_custom_lookup(cuckoo_hash_table_t *cht,
                              void *(*lookup)(cuckoo_hash_table_t *cht, void *key));

int32_t cuckoo_size(cuckoo_hash_table_t *cht);
int32_t cuckoo_num_elems(cuckoo_hash_table_t *cht);

void cuckoo_insert(cuckoo_hash_table_t *cht, void *key, void *val);
void cuckoo_remove(cuckoo_hash_table_t *cht, void *key);

/* Lookup is replaceable */
/* void *cuckoo_lookup(cuckoo_hash_table_t *cht, void *key); */
#define cuckoo_lookup(cht, key) (((struct cuckoo_hash_table_base *)(cht))->lookup(cht, key))

/* Set table size */
void cuckoo_resize(cuckoo_hash_table_t *ct, uint32_t new_size);

/* Growth coefficient -- how much does the table grow when it needs to be resized?.
   Default value is for slow growth, hoping that the table will be densely filled.
*/
#define CUCKOO_DEFAULT_GROWTH_COEFFICIENT 1.05678f
void cuckoo_set_growth_coefficient(cuckoo_hash_table_t *cht, float coeff);

/* How hard are we trying to rearrange entries before we decide to resize?
   Default is very hard indeed, for slow operation but high density.
*/
#define CUCKOO_DEFAULT_SEARCH_DEPTH 20
void cuckoo_set_search_depth(cuckoo_hash_table_t *cht, int depth);


typedef int cuckoo_iterator_t;

cuckoo_iterator_t make_cuckoo_iterator(cuckoo_hash_table_t *cht);
cuckoo_iterator_t cuckoo_iterator_step(cuckoo_hash_table_t *cht, cuckoo_iterator_t cit);
bool cuckoo_iterator_valid(cuckoo_hash_table_t *cht, cuckoo_iterator_t cit);

void *cuckoo_iterator_key(cuckoo_hash_table_t *cht, cuckoo_iterator_t cit);
void *cuckoo_iterator_value(cuckoo_hash_table_t *cht, cuckoo_iterator_t cit);

/* Result is to be free()-d */
void *cuckoo_get_values(cuckoo_hash_table_t *cht, int32_t *sizep);

/* With write_keys = false, upon reading back, the hashtable will be read-only */
size_t cuckoo_write(FILE *fs, cuckoo_hash_table_t *cht, bool write_keys);
cuckoo_hash_table_t *cuckoo_read(FILE *fs, int32_t key_size, int32_t entry_size,
                                 int nhashes, cuckoo_hash_ft **hashes, cuckoo_check_ft *check,
                                 void *aux);

bool cuckoo_is_readonly(cuckoo_hash_table_t *cht);

/* This macro can be used to speed up lookup, in the hope that hash and check functions
   will be inlined.
*/
#define CUSTOM_LOOKUP(name, hash1, hash2, hash3, check, zero_check)	\
  void *name(cuckoo_hash_table_t *cht, void *key)       		\
  {                                                     		\
    uint32_t hix;                                       		\
    void *entryp;                                       		\
    hix = hash1(key, cht->aux) % cht->size;                             \
    entryp = cuckoo_entryp(cht, hix);					\
    if (!zero_check(cht, entryp) && check(key, entryp, cht->aux)) {     \
      return entryp;							\
    }									\
    hix = hash2(key, cht->aux) % cht->size;                             \
    entryp = cuckoo_entryp(cht, hix);                                   \
    if (!zero_check(cht, entryp) && check(key, entryp, cht->aux)) {     \
      return entryp;							\
    }									\
    hix = hash3(key, cht->aux) % cht->size;                             \
    entryp = cuckoo_entryp(cht, hix);                                   \
    if (!zero_check(cht, entryp) && check(key, entryp, cht->aux)) {     \
      return entryp;							\
    }									\
    return NULL;                                                        \
  }

/* From here on, information should be private. It is only made public to enable CUSTOM_LOOKUP */

struct cuckoo_hash_table {
  struct cuckoo_hash_table_base;

  uint32_t size;
  uint32_t nelem;

  int32_t nhashes;
  cuckoo_hash_ft **hashes;

  cuckoo_check_ft *check;

  /*
     These are both small numbers, but they are used with big indices,
     so make sure proper index arithmetic is always applied.
  */
  size_t key_size;
  size_t entry_size;

  void *aux;

  void *keys;	/* NULL in a readonly table */
  void *entries;

  /* Cached to save mallocs */
  void *zero_entry;

  float growth_coefficient;
  int search_depth;
};

static inline void *
cuckoo_keyp(cuckoo_hash_table_t *cht, uint32_t ix)
{
  return (char*)cht->keys + cht->key_size * ix;
}

static inline void *
cuckoo_entryp(cuckoo_hash_table_t *cht, uint32_t ix)
{
  return (char*)cht->entries + cht->entry_size * ix;
}

static inline bool
cuckoo_is_zero_entry(cuckoo_hash_table_t *cht, void *ep)
{
  return 0 == memcmp(cht->zero_entry, ep, cht->entry_size);
}
