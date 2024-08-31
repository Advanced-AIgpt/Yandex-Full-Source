/**
 * @file util/ofst-symbol-table.c
 *
 * @date 2014
 *
 * @author Georgy Bronnikov <gogabr@yandex-team.ru>
 *
 * @copyright 2014 Yandex. All rights reserved.
 *
 * Using OpenFST symbol tables.
 */

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "util.h"
#include "log.h"
#include "cuckoo-hash.h"
#include "xalloc.h"

#include "ofst-symbol-table.h"

struct ofst_symbol_table {
  int ncodes;
  char **code2sym;
  cuckoo_hash_table_t *sym2code;
};

extern cuckoo_hash_ft *ofst_string_hashes[]; /* static declaration rejected by MSVC. */
static bool string_check(void *key, void *val, void *aux);

static char *read_string(FILE *fs, err_t *err);

struct hashelem {
  char *s;
  int code;
};

static int32_t const MAGIC = 2125658996;
static int32_t const MAX_SANE_CODE = 32768;

ofst_symbol_table_t *
ofst_symbol_table_read(char *fname)
{
  FILE *fs;
  int r;
  int32_t magic;
  char *name;
  int64_t available_key;
  int64_t size;
  ofst_symbol_table_t *ost = NULL;
  err_t err = ERR_OK;

  fs = fopen(fname, "rb");
  if (fs == NULL) {
    log_warning("%s: could not open %s", __FUNCTION__, fname);
    return NULL;
  }

  ost = xcalloc(1, sizeof(ofst_symbol_table_t));

  r = fread(&magic, sizeof(int32_t), 1, fs);
  if (r != 1) {
    log_warning("%s: IO error", __FUNCTION__);
    goto BAD;
  }
  if (magic != MAGIC) {
    log_warning("%s: bad magic", __FUNCTION__);
    goto BAD;
  }
  name = read_string(fs, &err);
  free(name); // never used
  if (err != ERR_OK) goto BAD;

  r = fread(&available_key, sizeof(int64_t), 1, fs);
  if (r != 1) {
    log_warning("%s: IO error", __FUNCTION__);
    goto BAD;
  }

  r = fread(&size, sizeof(int64_t), 1, fs);
  if (r != 1) {
    log_warning("%s: IO error", __FUNCTION__);
    goto BAD;
  }

  ost->ncodes = (int)size;
  ost->code2sym = xcalloc(ost->ncodes, sizeof(char*));
  ost->sym2code = make_cuckoo_hash_table(sizeof(char*), sizeof(struct hashelem),	// key_size, entry_size
                                         3, ofst_string_hashes, string_check, 	    // nhashes, hashes, check
                                         NULL);						// aux
  cuckoo_set_growth_coefficient(ost->sym2code, 1.5); // grow fast

  {
    int i;
    for (i = 0; i < size; i++) {
      char *symbol;
      int64_t code;
      struct hashelem he;
      memset(&he, 0, sizeof(he));

      symbol = read_string(fs, &err);
      r = fread(&code, sizeof(int64_t), 1, fs);
      if (err != ERR_OK || r != 1) {
        log_warning("%s: IO error", __FUNCTION__);
        goto BAD;
      }

      if (code > MAX_SANE_CODE) {
        log_warning("%s: improbable code value %d", __FUNCTION__, code);
        free(symbol);
        goto BAD;
      }
      if (code >= ost->ncodes) {
        int j;
        ost->code2sym = xrealloc(ost->code2sym, (size_t)(code+1) * sizeof(char*));
        for (j = ost->ncodes; j <= code; j++) {
          ost->code2sym[j] = NULL;
        }
      }
      ost->code2sym[code] = symbol;
      he.code = (int)code;
      he.s = symbol;
      cuckoo_insert(ost->sym2code, &symbol, &he);
    }
  }

  fclose(fs);
  return ost;

BAD:
  fclose(fs);
  ofst_symbol_table_free(ost);
  return NULL;
}

ofst_symbol_table_t *
ofst_symbol_table_read_from_blob(mem_blob_t blob)
{
  FILE *fs;
  int r;
  int32_t magic;
  char *name;
  int64_t available_key;
  int64_t size;
  ofst_symbol_table_t *ost = NULL;
  err_t err = ERR_OK;

  fs = fmemopen(blob.data, blob.size, "rb");
  if (fs == NULL) {
    log_warning("%s: could not open memory at address %p", __FUNCTION__, blob.data);
    return NULL;
  }

  ost = xcalloc(1, sizeof(ofst_symbol_table_t));

  r = fread(&magic, sizeof(int32_t), 1, fs);
  if (r != 1) {
    log_warning("%s: IO error", __FUNCTION__);
    goto BAD;
  }
  if (magic != MAGIC) {
    log_warning("%s: bad magic", __FUNCTION__);
    goto BAD;
  }
  name = read_string(fs, &err);
  free(name); // never used
  if (err != ERR_OK) goto BAD;

  r = fread(&available_key, sizeof(int64_t), 1, fs);
  if (r != 1) {
    log_warning("%s: IO error", __FUNCTION__);
    goto BAD;
  }

  r = fread(&size, sizeof(int64_t), 1, fs);
  if (r != 1) {
    log_warning("%s: IO error", __FUNCTION__);
    goto BAD;
  }

  ost->ncodes = (int)size;
  ost->code2sym = xcalloc(ost->ncodes, sizeof(char*));
  ost->sym2code = make_cuckoo_hash_table(sizeof(char*), sizeof(struct hashelem),	// key_size, entry_size
                                         3, ofst_string_hashes, string_check, 	    // nhashes, hashes, check
                                         NULL);						// aux
  cuckoo_set_growth_coefficient(ost->sym2code, 1.5); // grow fast

  {
    int i;
    for (i = 0; i < size; i++) {
      char *symbol;
      int64_t code;
      struct hashelem he;
      memset(&he, 0, sizeof(he));

      symbol = read_string(fs, &err);
      r = fread(&code, sizeof(int64_t), 1, fs);
      if (err != ERR_OK || r != 1) {
        free(symbol);
        log_warning("%s: IO error", __FUNCTION__);
        goto BAD;
      }

      if (code > MAX_SANE_CODE) {
        log_warning("%s: improbable code value %d", __FUNCTION__, code);
        free(symbol);
        goto BAD;
      }
      if (code >= ost->ncodes) {
        int j;
        ost->code2sym = xrealloc(ost->code2sym, (size_t)(code+1) * sizeof(char*));
        for (j = ost->ncodes; j <= code; j++) {
          ost->code2sym[j] = NULL;
        }
      }
      ost->code2sym[code] = symbol;
      he.code = (int)code;
      he.s = symbol;
      cuckoo_insert(ost->sym2code, &symbol, &he);
    }
  }

  fclose(fs);
  return ost;

BAD:
  fclose(fs);
  ofst_symbol_table_free(ost);
  return NULL;
}

void
ofst_symbol_table_free(ofst_symbol_table_t *ost)
{
  int i;

  if (ost == NULL) return;

  cuckoo_free(ost->sym2code);

  for (i = 0; i < ost->ncodes; i++) {
    free(ost->code2sym[i]);
  }
  free(ost->code2sym);
  free(ost);
}

int
ofst_symbol_table_size(ofst_symbol_table_t *ost)
{
  return ost->ncodes;
}

char *
ofst_symbol_table_symbol(ofst_symbol_table_t *ost, int code)
{
  if (code < 0 || code >= ost->ncodes) {
    log_warning("%s: bad code %d", __FUNCTION__, code);
    return NULL;
  }
  if (code == 0) return NULL;

  return ost->code2sym[code];
}

int
ofst_symbol_table_code(ofst_symbol_table_t *ost, char *symbol)
{
  struct hashelem *he = cuckoo_lookup(ost->sym2code, &symbol);
  if (he == NULL) return -1;
  return he->code;
}

enum {
  PRIME1 = 1192199,
  PRIME2 = 1197619,
  PRIME3 = 1203217,
};

static uint32_t
string_hash1(void *key, void *aux)
{
  char *s = *(char**)key;
  uint32_t res = 0;
  for (; *s; s++) {
    res = res * PRIME1 + *s * PRIME1;
  }
  return res;

  UNUSED(aux);
}

static uint32_t
string_hash2(void *key, void *aux)
{
  char *s = *(char**)key;
  uint32_t res = 0;
  for (; *s; s++) {
    res = res * PRIME2 + *s * PRIME2;
  }
  return res;

  UNUSED(aux);
}

static uint32_t
string_hash3(void *key, void *aux)
{
  char *s = *(char**)key;
  uint32_t res = 0;
  for (; *s; s++) {
    res = res * PRIME3 + *s * PRIME3;
  }
  return res;

  UNUSED(aux);
}

cuckoo_hash_ft *ofst_string_hashes[] = {
  string_hash1,
  string_hash2,
  string_hash3
};

bool
string_check(void *key, void *val, void *aux)
{
  char *s = *(char**)key;
  struct hashelem *he = val;

  return (strcmp(s, he->s) == 0);

  UNUSED(aux);
}

/* OpenFST way of representing strings. */
static char *
read_string(FILE *fs, err_t *err)
{
  int r;
  int32_t len;
  char *s;

  if (*err != ERR_OK) return NULL;

  r = fread(&len, sizeof(int32_t), 1, fs);
  if (r != 1) {
    log_warning("%s: IO error", __FUNCTION__);
    *err = ERR_IO;
    return NULL;
  }

  s = xmalloc(len+1);
  r = fread(s, 1, len, fs);
  if (r != len) {
    log_warning("%s: IO error", __FUNCTION__);
    free(s);
    *err = ERR_IO;
    return NULL;
  }
  s[len] = '\0';
  return s;
}
