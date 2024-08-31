/**
 * @file util/cuckoo-hash.c
 *
 * @date 2014
 *
 * @author Georgy Bronnikov <gogabr@yandex-team.ru>
 *
 * @copyright 2014 Yandex. All rights reserved.
 *
 * Cuckoo hash implementation.
 */

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "log.h"
#include "xalloc.h"
#include "util.h"

#include "cuckoo-hash.h"

#define CUCKOO_DEBUG false

#define MIN_SIZE 101

static void *cuckoo_lookup_dflt(cuckoo_hash_table_t *cht, void *key);

/* Keys and entries are memcpy'd when inserted. */
cuckoo_hash_table_t *
make_cuckoo_hash_table(int32_t key_size, int32_t entry_size,
                       int nhashes, cuckoo_hash_ft **hashes, cuckoo_check_ft *check,
                       void *aux)
{
  cuckoo_hash_table_t *cht = xcalloc(1, sizeof(cuckoo_hash_table_t));

  cht->lookup = cuckoo_lookup_dflt;

  cht->size = MIN_SIZE; /* Start low to exercise code that grows the table */
  cht->nelem = 0;
  cht->nhashes = nhashes;
  cht->hashes = hashes;
  cht->check = check;
  cht->key_size = key_size;
  cht->entry_size = entry_size;
  cht->aux = aux;
  cht->keys = xcalloc(cht->size, key_size);
  cht->entries = xcalloc(cht->size, entry_size);
  cht->zero_entry = xcalloc(1, entry_size);
  cht->growth_coefficient = CUCKOO_DEFAULT_GROWTH_COEFFICIENT;
  cht->search_depth = CUCKOO_DEFAULT_SEARCH_DEPTH;

  return cht;
}

void
cuckoo_free(cuckoo_hash_table_t *cht)
{
  if (cht == NULL) return;

  free(cht->keys);
  free(cht->entries);
  free(cht->zero_entry);

  free(cht);
}

void
cuckoo_set_custom_lookup(cuckoo_hash_table_t *cht,
                         void *(*lookup)(cuckoo_hash_table_t *cht, void *key))
{
  cht->lookup = lookup;
}

int32_t
cuckoo_size(cuckoo_hash_table_t *cht)
{
  return cht->size;
}

int32_t
cuckoo_num_elems(cuckoo_hash_table_t *cht)
{
  return cht->nelem;
}

void
cuckoo_remove(cuckoo_hash_table_t *cht, void *key)
{
  void *entryp;

  if (cuckoo_is_readonly(cht)) {
    log_error("%s: readonly table", __FUNCTION__);
    return;
  }

  entryp = cht->lookup(cht, key);
  if (entryp != NULL) {
    memset(entryp, 0, cht->entry_size);
    cht->nelem--;
  }

  /* No need to wipe out the key */
}

static void *
cuckoo_lookup_dflt(cuckoo_hash_table_t *cht, void *key)
{
  int hno;
  for (hno = 0; hno < cht->nhashes; hno++) {
    uint32_t hix = cht->hashes[hno](key, cht->aux) % cht->size;
    void *entryp = cuckoo_entryp(cht, hix);
    if (!cuckoo_is_zero_entry(cht, entryp) && cht->check(key, entryp, cht->aux)) {
      return entryp;
    }
  }
  return NULL;
}

char const * const MAGIC = "CHT ";

/* With write_keys = false, upon reading back, the hashtable will be read-only */
size_t
cuckoo_write(FILE *fs, cuckoo_hash_table_t *cht, bool write_keys)
{
  char *errstr = "%s: error writing";
  size_t v;
  size_t tot = 0;

  v = fwrite(MAGIC, 1, 4, fs);
  if (v != 4) {
    log_warning(errstr, __FUNCTION__);
    return -1;
  }
  tot += 4;

  v = fwrite(&write_keys, sizeof(bool), 1, fs);
  if (v != 1) {
    log_warning(errstr, __FUNCTION__);
    return -1;
  }
  tot += sizeof(bool);

  v = fwrite(&cht->size, sizeof(uint32_t), 1, fs);
  if (v != 1) {
    log_warning(errstr, __FUNCTION__);
    return -1;
  }
  tot += sizeof(int32_t);

  v = fwrite(&cht->nelem, sizeof(uint32_t), 1, fs);
  if (v != 1) {
    log_warning(errstr, __FUNCTION__);
    return -1;
  }
  tot += sizeof(int32_t);

  if (write_keys) {
    v = fwrite(cht->keys, cht->key_size, cht->size, fs);
    if (v != cht->size) {
      log_warning(errstr, __FUNCTION__);
      return -1;
    }
    tot += cht->key_size * cht->size;
  }

  v = fwrite(cht->entries, cht->entry_size, cht->size, fs);
  if (v != cht->size) {
    log_warning(errstr, __FUNCTION__);
    return -1;
  }
  tot += cht->entry_size * cht->size;

  return tot;
}

cuckoo_hash_table_t *
cuckoo_read(FILE *fs, int32_t key_size, int32_t entry_size,
            int nhashes, cuckoo_hash_ft **hashes, cuckoo_check_ft *check,
            void *aux)
{
  char *errstr = "%s: error reading";
  size_t v;

  cuckoo_hash_table_t *cht = NULL;
  bool have_keys;
  int32_t size;
  int32_t nelem;
  char magic_buf[5];

  v = fread(magic_buf, 1, 4, fs);
  if (v != 4) {
    log_warning(errstr, __FUNCTION__);
    goto BAD;
  }
  magic_buf[4] = '\0';
  if (strcmp(magic_buf, MAGIC) != 0) {
    log_warning("%s: bad magic", __FUNCTION__);
    goto BAD;
  }

  v = fread(&have_keys, sizeof(bool), 1, fs);
  if (v != 1) {
    log_warning(errstr, __FUNCTION__);
    goto BAD;
  }

  v = fread(&size, sizeof(uint32_t), 1, fs);
  if (v != 1) {
    log_warning(errstr, __FUNCTION__);
    goto BAD;
  }

  v = fread(&nelem, sizeof(uint32_t), 1, fs);
  if (v != 1) {
    log_warning(errstr, __FUNCTION__);
    goto BAD;
  }

  cht = xcalloc(1, sizeof(cuckoo_hash_table_t));
  cht->lookup = cuckoo_lookup_dflt;

  cht->size = size;
  cht->nelem = nelem;
  cht->nhashes = nhashes;
  cht->hashes = hashes;
  cht->check = check;
  cht->key_size = key_size;
  cht->entry_size = entry_size;
  cht->aux = aux;
  cht->zero_entry = xcalloc(1, entry_size);

  if (have_keys) {
    cht->keys = xcalloc(cht->size, cht->key_size);
    v = fread(cht->keys, cht->key_size, cht->size, fs);
    if (v != cht->size) {
      log_warning(errstr, __FUNCTION__);
      goto BAD;
    }
  }

  cht->entries = xcalloc(cht->size, cht->entry_size);
  v = fread(cht->entries, cht->entry_size, cht->size, fs);
  if (v != cht->size) {
    log_warning(errstr, __FUNCTION__);
    goto BAD;
  }

  return cht;

BAD:
  cuckoo_free(cht);
  return NULL;
}

bool
cuckoo_is_readonly(cuckoo_hash_table_t *cht)
{
  return (cht->keys == NULL);
}

/* Insert is the only non-trivial operation. */

/* A list of indices we are not allowed to touch when reorganizing entries.
   In insert_helper, this list is built on the stack.
*/
struct skip_list {
  uint32_t index;
  struct skip_list *next;
};

static void checked_insert(cuckoo_hash_table_t *cht, void *key, void *val);
static bool index_in_list(uint32_t ix, struct skip_list *l);
static bool insert_helper(cuckoo_hash_table_t *cht,
                          void *new_key, void *new_entry,
                          int level, struct skip_list *to_skip);
static uint32_t next_size(cuckoo_hash_table_t *cht);
static void move_entries(cuckoo_hash_table_t *cht,
                         uint32_t old_size, void *old_keys, void *old_entries);

void
cuckoo_insert(cuckoo_hash_table_t *cht, void *key, void *val)
{
  if (cuckoo_is_readonly(cht)) {
    log_error("%s: readonly table", __FUNCTION__);
    return;
  }

  checked_insert(cht, key, val);
  cht->nelem++;
}

static void
checked_insert(cuckoo_hash_table_t *cht, void *key, void *val)
{
  bool success;
  success = insert_helper(cht, key, val,
                          0, NULL);
  if (!success) {
    uint32_t new_size = next_size(cht);
    if (new_size == 0) {
      /* Size table exhausted */
      log_warning("%s: cannot grow any further", __FUNCTION__);
    }

    cuckoo_resize(cht, new_size);
    checked_insert(cht, key, val);
  }
}

static bool
index_in_list(uint32_t ix, struct skip_list *list)
{
  struct skip_list *ll;
  for (ll = list; ll != NULL; ll = ll->next) {
    if (ll->index == ix) {
      return true;
    }
  }
  return false;
}

static bool
insert_helper(cuckoo_hash_table_t *cht,
              void *new_key, void *new_entry,
              int level, struct skip_list *to_skip)
{
  int hno;

  if (level >= cht->search_depth) return false;

  /* Look for an empty slot */
  for (hno = 0; hno < cht->nhashes; hno++) {
    uint32_t hix = cht->hashes[hno](new_key, cht->aux) % cht->size;
    void *entryp = cuckoo_entryp(cht, hix);
    if (cuckoo_is_zero_entry(cht, entryp)) {
      void *keyp = cuckoo_keyp(cht, hix);
      memcpy(keyp, new_key, cht->key_size);
      memcpy(entryp, new_entry, cht->entry_size);
      return true;
    }
  }

  /* No empty slot; try to make one by calling ourselves recursively */
  for (hno = 0; hno < cht->nhashes; hno++) {
    uint32_t hix = cht->hashes[hno](new_key, cht->aux) % cht->size;
    void *entryp = cuckoo_entryp(cht, hix);
    void *keyp = cuckoo_keyp(cht, hix);
    struct skip_list new_to_skip = { hix, to_skip };

    /* Move out the old value to a different place */
    if (!index_in_list(hix, to_skip) &&
        insert_helper(cht, keyp, entryp,
                      level + 1, &new_to_skip)) {
      memcpy(keyp, new_key, cht->key_size);
      memcpy(entryp, new_entry, cht->entry_size);
      return true;
    }
  }

  /* All attempts failed */
  return false;
}

static uint32_t
next_size(cuckoo_hash_table_t *cht)
{
  return (uint32_t)(cht->size * cht->growth_coefficient);
}

void
cuckoo_resize(cuckoo_hash_table_t *cht, uint32_t new_size)
{
  uint32_t old_size = cht->size;
  void *old_keys = cht->keys;
  void *old_entries = cht->entries;

  if (CUCKOO_DEBUG) log_info("%s: from %u to %u", __func__, old_size, new_size);

  if (new_size < MIN_SIZE) {
    new_size = MIN_SIZE;
  }
  if (new_size < cht->nelem) {
    log_warning("%s: size less than number of entries: %d < %d, ignoring",
                __FUNCTION__, new_size, cht->nelem);
    return;
  }
  if (new_size < old_size) {
    log_warning("%s: new size is less than the old one: %d < %d ignoring",
                __FUNCTION__, new_size, old_size);
    return;
  }

  cht->size = new_size;
  cht->keys = xcalloc(cht->size, cht->key_size);
  cht->entries = xcalloc(cht->size, cht->entry_size);

  /* NB! calls to checked_insert inside move_entries might lead to recursive
     invocation of grow_table.
  */
  move_entries(cht, old_size, old_keys, old_entries);
  free(old_keys);
  free(old_entries);
}

static void
move_entries(cuckoo_hash_table_t *cht,
             uint32_t old_size, void *old_keys, void *old_entries)
{
  uint32_t i;
  ///**/log_info("--- --- ---");
  for (i = 0; i < old_size; i++) {
    void *okp = (char*)old_keys + i * cht->key_size;
    void *oep = (char*)old_entries + i * cht->entry_size;

    if (!cuckoo_is_zero_entry(cht, oep)) {
      ///**/ log_info("Moving %p/%s", okp, *(char**)okp);
      checked_insert(cht, okp, oep);
    }
  }
}

void
cuckoo_set_growth_coefficient(cuckoo_hash_table_t *cht, float coeff)
{
  cht->growth_coefficient = coeff;
}

void
cuckoo_set_search_depth(cuckoo_hash_table_t *cht, int depth)
{
  cht->search_depth = depth;
}

static int const ITER_INVALID = -1;

cuckoo_iterator_t
make_cuckoo_iterator(cuckoo_hash_table_t *cht)
{
  uint32_t i;

  if (cht->nelem == 0) {
    return ITER_INVALID;
  }

  for (i = 0; i < cht->size; i++) {
    if (!cuckoo_is_zero_entry(cht, cuckoo_entryp(cht, i))) {
      return i;
    }
  }

  log_error("%s: no entries found though cht->size == %u", __FUNCTION__, cht->size);
  return ITER_INVALID;
}

cuckoo_iterator_t
cuckoo_iterator_step(cuckoo_hash_table_t *cht, cuckoo_iterator_t cit)
{
  uint32_t i;
  for (i = cit + 1; i < cht->size; i++) {
    if (!cuckoo_is_zero_entry(cht, cuckoo_entryp(cht, i))) {
      return i;
    }
  }
  return ITER_INVALID;
}

bool
cuckoo_iterator_valid(cuckoo_hash_table_t *cht, cuckoo_iterator_t cit)
{
  return (cit != ITER_INVALID);

  UNUSED(cht);
}

void *
cuckoo_iterator_key(cuckoo_hash_table_t *cht, cuckoo_iterator_t cit)
{
  if (cht->keys == NULL || cit == ITER_INVALID) {
    return NULL;
  } else {
    return cuckoo_keyp(cht, cit);
  }
}

void *
cuckoo_iterator_value(cuckoo_hash_table_t *cht, cuckoo_iterator_t cit)
{
  if (cht->keys == NULL || cit == ITER_INVALID) {
    return NULL;
  } else {
    return cuckoo_entryp(cht, cit);
  }
}

void *
cuckoo_get_values(cuckoo_hash_table_t *cht, int32_t *sizep)
{
  if (cht->nelem == 0) {
    *sizep = 0;
    return NULL;
  } else {
    void *res = xcalloc(cht->nelem, cht->entry_size);
    uint32_t n = 0;
    uint32_t i;
    for (i = 0; i < cht->size; i++) {
      void *e = cuckoo_entryp(cht, i);
      if (!cuckoo_is_zero_entry(cht, e)) {
        memcpy((char*)res + n * cht->entry_size, e, cht->entry_size);
        n++;
      }
    }
    if (n != cht->nelem) {
      log_warning("%s: nelem value is wrong: %d != %d",
                  __FUNCTION__, n, cht->nelem);
    }
    *sizep = n;
    return res;
  }
}
