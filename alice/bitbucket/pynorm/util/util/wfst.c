/**
 * @file util/wfst.c
 *
 * @date 2014
 *
 * @author Georgy Bronnikov <gogabr@yandex-team.ru>
 *
 * @copyright 2014 Yandex. All rights reserved.
 *
 * Weighted Finite State Transducers; OpenFST's "const" format.
 */

#undef _GNU_SOURCE
#define _GNU_SOURCE /* On older Gcc, includes `stpcpy()`. */

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "zlib.h"

#include "config.h"
#include "log.h"
#include "util.h"
#include "xalloc.h"

#include "ofst-symbol-table.h"

#include "wfst.h"

enum wfst_flags {
  HAS_ISYMBOLS = 0x1,
  HAS_OSYMBOLS = 0x2,
  IS_ALIGNED = 0x4
};

typedef struct wfst_header {
  char *fst_type;
  char *arc_type;
  int32_t version;
  int32_t flags;
  uint64_t properties;
  int64_t start;
  int64_t num_states;
  int64_t num_arcs;
} wfst_header_t;

typedef struct wfst_state {
  REAL final_cost;
  int32_t pos;
  int32_t num_arcs;
  int32_t num_iepsilons;
  int32_t num_oepsilons;
} wfst_state_t;

struct wfst {
  char *fname;
  wfst_header_t *header;
  wfst_state_t *states;
  wfst_arc_t *arcs;
  unsigned char *memory_buffer;
};

static const int32_t FST_MAGIC_NUMBER = 2125659606;
static const int32_t FILE_ALIGNMENT = 16;
enum {
   CHUNK_SIZE = 32768
};
// 10 would be enough, given we are only reading fst and arc types,
// but let's be generous.
static const int32_t MAX_STRING_LEN = 4096;

static void *read_header(void *wfst_data, wfst_header_t **header);
static void free_header(wfst_header_t *h);
static void *read_string(void *data, char **string);
void *up_align(void *data, long pos);

bool
is_data_compressed(unsigned char *data)
{
  return (data != NULL) && (data[0] == 0x1f) && (data[1] == 0x8b);
}

unsigned char *
decompress_data(unsigned char *data, int32_t data_size, int32_t *decompressed_size)
{
  int32_t ret;
  unsigned have;
  z_stream strm;
  unsigned char out[CHUNK_SIZE];
  unsigned char *decompressed_data = xmalloc(sizeof(char) * data_size);
  int32_t result_size = 0;

  /* allocate inflate state */
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  strm.avail_in = 0;
  strm.next_in = Z_NULL;
  // windowBits parameter is the base two logarithm of the maximum window size. set it to 15
  // add 32 enable zlib and gzip decoding with automatic header detection
  ret = inflateInit2(&strm, 15 + 32);
  if (ret != Z_OK)
    goto BAD;

  strm.avail_in = data_size;
  strm.next_in = data;

  do {
    strm.avail_out = CHUNK_SIZE;
    strm.next_out = out;
    ret = inflate(&strm, Z_NO_FLUSH);
    assert(ret != Z_STREAM_ERROR);
    switch (ret) {
      case Z_NEED_DICT:
      case Z_DATA_ERROR:
      case Z_MEM_ERROR:
        goto BAD;
    }
    have = CHUNK_SIZE - strm.avail_out;
    decompressed_data = xrealloc(decompressed_data, result_size + have);
    memcpy(decompressed_data + result_size, out, have);
    result_size += have;
  } while (strm.avail_out == 0);

  (void)inflateEnd(&strm);
  *decompressed_size = result_size;
  return decompressed_data;

BAD:
  (void)inflateEnd(&strm);
  free(decompressed_data);
  return NULL;
}

static int
wfst_label_compare(void const *a, void const *b)
{
  wfst_arc_t *ta = (wfst_arc_t*)a; /* MSVC is unhappy without explicit conversion. */
  wfst_arc_t *tb = (wfst_arc_t*)b;

  if (ta->ilabel < tb->ilabel) {
    return -1;
  } else if (ta->ilabel > tb->ilabel) {
    return 1;
  } else {
    return 0;
  }
}

void wfst_sort_by_ilabels(wfst_t *wfst) {
  for (int state_id = 0; state_id < wfst->header->num_states; state_id++) {
    int num_arcs_p = wfst->states[state_id].num_arcs;
    int pos = wfst->states[state_id].pos;
    wfst_arc_t* arcs = &wfst->arcs[pos];
    qsort(arcs, num_arcs_p, sizeof(wfst_arc_t), wfst_label_compare);
  }
}

wfst_t *
wfst_read(char *fname)
{
  int32_t num_states;
  int32_t num_arcs;
  wfst_t *wfst;
  int32_t wfst_data_size;
  unsigned char *decompressed_data;
  int32_t decompressed_size;
  unsigned char *wfst_data = read_whole_file(fname, &wfst_data_size);
  unsigned char *current_pos;
  if (wfst_data == NULL) {
    log_error("Could not read file %s", fname);
    return NULL;
  }

  if (is_data_compressed(wfst_data)) {
    decompressed_data = decompress_data(wfst_data, wfst_data_size, &decompressed_size);
    free(wfst_data);
    wfst_data = decompressed_data;
    wfst_data_size = decompressed_size;
  }
  current_pos = wfst_data;

  wfst = xmalloc(sizeof(wfst_t));
  memset(wfst, 0, sizeof(wfst_t));

  wfst->memory_buffer = wfst_data;
  wfst->fname = extract_filename_from_path(fname);

  current_pos = read_header(current_pos, &wfst->header);
  if(wfst->header == NULL) {
    log_error("Could not read wfst header from %s!", fname);
    goto BAD;
  }

  if (wfst->header->flags & IS_ALIGNED) {
    current_pos = up_align(current_pos, current_pos - wfst_data);
    if (current_pos - wfst_data > wfst_data_size) {
      goto BAD;
    }
  }

  num_states = (int32_t)wfst->header->num_states;
  num_arcs = (int32_t)wfst->header->num_arcs;

  wfst->states = (wfst_state_t*)current_pos;
  current_pos += sizeof(wfst_state_t) * num_states;

  if (wfst->header->flags & IS_ALIGNED) {
    current_pos = up_align(current_pos, current_pos - wfst_data);
    if (current_pos - wfst_data > wfst_data_size) {
      goto BAD;
    }
  }

  wfst->arcs = (wfst_arc_t*)current_pos;
  current_pos += sizeof(wfst_arc_t) * num_arcs;

  if (current_pos - wfst_data > wfst_data_size) {
    log_error("Could not create wfst from file %s! Too small data!", fname);
    goto BAD;
  }
  wfst_sort_by_ilabels(wfst);
  return wfst;

BAD:
  wfst_free(wfst);
  return NULL;
}

wfst_t *
wfst_read_from_blob(const char *fst_name, mem_blob_t blob)
{
  int32_t num_states;
  int32_t num_arcs;
  wfst_t *wfst;
  int32_t wfst_data_size;
  unsigned char *decompressed_data;
  int32_t decompressed_size;
  unsigned char *wfst_data = read_whole_blob(blob);
  wfst_data_size = blob.size;
  unsigned char *current_pos;
  if (wfst_data == NULL) {
    log_error("Could not read blob at address %p", blob.data);
    return NULL;
  }

  if (is_data_compressed(wfst_data)) {
    decompressed_data = decompress_data(wfst_data, wfst_data_size, &decompressed_size);
    free(wfst_data);
    wfst_data = decompressed_data;
    wfst_data_size = decompressed_size;
  }
  current_pos = wfst_data;

  wfst = xcalloc(1, sizeof(wfst_t));

  wfst->memory_buffer = wfst_data;
  wfst->fname = xstrdup(fst_name);

  current_pos = read_header(current_pos, &wfst->header);
  if(wfst->header == NULL) {
    log_error("Could not read wfst header from blob %p!", blob.data);
    goto BAD;
  }

  if (wfst->header->flags & IS_ALIGNED) {
    current_pos = up_align(current_pos, current_pos - wfst_data);
    if (current_pos - wfst_data > wfst_data_size) {
      goto BAD;
    }
  }

  num_states = (int32_t)wfst->header->num_states;
  num_arcs = (int32_t)wfst->header->num_arcs;

  wfst->states = (wfst_state_t*)current_pos;
  current_pos += sizeof(wfst_state_t) * num_states;

  if (wfst->header->flags & IS_ALIGNED) {
    current_pos = up_align(current_pos, current_pos - wfst_data);
    if (current_pos - wfst_data > wfst_data_size) {
      goto BAD;
    }
  }

  wfst->arcs = (wfst_arc_t*)current_pos;
  current_pos += sizeof(wfst_arc_t) * num_arcs;

  if (current_pos - wfst_data > wfst_data_size) {
    log_error("Could not create wfst from blob %p! Too small data!", blob.data);
    goto BAD;
  }
  wfst_sort_by_ilabels(wfst);
  return wfst;

BAD:
  wfst_free(wfst);
  return NULL;
}

void
wfst_free(wfst_t *wfst)
{
  if (wfst == NULL) return;

  free(wfst->memory_buffer);
  free_header(wfst->header);
  free(wfst->fname);
  free(wfst);
}

char *
wfst_fname(wfst_t *wfst)
{
  return wfst->fname;
}

int32_t
wfst_num_states(wfst_t *wfst)
{
  return (int32_t)wfst->header->num_states;
}

int32_t
wfst_start_state(wfst_t *wfst)
{
  return (int32_t)wfst->header->start;
}

// INFINITY when the state is not final.
REAL
wfst_state_final_cost(wfst_t *wfst, int32_t state_id)
{
  assert(0 <= state_id && state_id < wfst->header->num_states);

  return wfst->states[state_id].final_cost;
}

wfst_arc_t *
wfst_state_arcs(wfst_t *wfst, int32_t state_id, int32_t *num_arcs_p)
{
  assert(0 <= state_id && state_id < wfst->header->num_states);

  *num_arcs_p = wfst->states[state_id].num_arcs;
  return &wfst->arcs[ wfst->states[state_id].pos ];
}

static void *
read_header(void *wfst_data, wfst_header_t **header)
{
  int32_t magic;

  wfst_header_t *h = xmalloc(sizeof(wfst_header_t));
  memset(h, 0, sizeof(wfst_header_t));

  memmove(&magic, wfst_data, sizeof(magic));
  wfst_data = (char*)wfst_data + sizeof(magic);
  if (magic != FST_MAGIC_NUMBER) {
    log_error("Bad FST magic %d %d", magic, FST_MAGIC_NUMBER);
    goto BAD;
  }

  wfst_data = read_string(wfst_data, &h->fst_type);
  if (h->fst_type == NULL || strcmp(h->fst_type, "const") != 0) {
    log_error("Bad FST type %s, only const is supported", h->fst_type);
    goto BAD;
  }

  wfst_data = read_string(wfst_data, &h->arc_type);
  if (h->arc_type == NULL || strcmp(h->arc_type, "standard") != 0) {
    log_error("Bad FST arc type %s, only standard is supported", h->arc_type);
    goto BAD;
  }

  memmove(&h->version, wfst_data, sizeof(h->version));
  wfst_data = (char*)wfst_data + sizeof(h->version);
  memmove(&h->flags, wfst_data, sizeof(h->flags));
  wfst_data = (char*)wfst_data + sizeof(h->flags);
  memmove(&h->properties, wfst_data, sizeof(h->properties));
  wfst_data = (char*)wfst_data + sizeof(h->properties);
  memmove(&h->start, wfst_data, sizeof(h->start));
  wfst_data = (char*)wfst_data + sizeof(h->start);
  memmove(&h->num_states, wfst_data, sizeof(h->num_states));
  wfst_data = (char*)wfst_data + sizeof(h->num_states);
  memmove(&h->num_arcs, wfst_data, sizeof(h->num_arcs));
  wfst_data = (char*)wfst_data + sizeof(h->num_arcs);

  *header = h;

  return wfst_data;

BAD:
  free_header(h);
  return NULL;
}

static void
free_header(wfst_header_t *h)
{
  if (h == NULL) return;

  free(h->fst_type);
  free(h->arc_type);
  free(h);
}

static void *
read_string(void *wfst_data, char **string)
{
  char *s;
  int32_t len;
  memmove(&len, wfst_data, sizeof(len));
  wfst_data = (char*)wfst_data + sizeof(len);
  if (len > MAX_STRING_LEN) {
    log_error("Bad string length in FST");
    return NULL;
  }

  s = xmalloc(len + 1);
  memmove(s, wfst_data, len);

  wfst_data = (char*)wfst_data + sizeof(char) * len;
  s[len] = '\0';
  *string = s;

  return wfst_data;
}

void *
up_align(void *data, long pos)
{
  long aligned_pos = pos;

  // align
  aligned_pos += FILE_ALIGNMENT - 1;
  aligned_pos -= aligned_pos % FILE_ALIGNMENT;

  data = (char*)data + aligned_pos - pos;
  return data;
}

static char *
skip_utf8_symbol(char *s, int max_len)
{
  char *ss = s;
  /* Assume the first symbol always starts a UTF8 sequence */
  ss++;
  while ((*ss & 0xc0) == 0x80 &&  /* continuation byte */
         ss - s < max_len) {      /* guard against pathological byte sequences */
    ss++;
  }
  return ss;
}

int *
str_to_labels(ofst_symbol_table_t *ost, char *str, int *lenp)
{
  int len = 0;
  int cap = 10;
  char *s;
  int replacement_code = ofst_symbol_table_code(ost, "-");
  int space_code = ofst_symbol_table_code(ost, " ");

  int *v = xcalloc(cap, sizeof(int));

  if ((replacement_code == -1) ||
      (space_code == -1)) {
    log_warning("%s: no code for space symbol or -");
    replacement_code = 1; /* more or less arbitrary; situation is pathologic anyway */
    space_code = 1;
  }

  s = str;
  while (*s) {
    int ov = -1;
    enum {
      MAX_LEN = 5
    };
    char buf[MAX_LEN+2];
    char *ss;
    for (ss = s; *ss && ss - s <= MAX_LEN; ss++) {
      strncpy(buf, s, ss + 1 - s);
      buf[ss + 1 - s] = '\0';
      ov = ofst_symbol_table_code(ost, buf);
      if (ov != -1) break;
    }

    if ((len == cap) ||
        ((ov == -1) && (len + 2 >= cap))) {
      v = xrealloc(v, sizeof(int) * 2 * cap);
      cap *= 2;
    }

    if (ov == -1) {
      ss = skip_utf8_symbol(s, MAX_LEN);
      strncpy(buf, s, ss - s);
      buf[ss - s] = '\0';
      log_warning("%s: unknown symbol %s", __FUNCTION__, buf);
      s = ss;
      v[len++] = space_code;
      v[len++] = replacement_code;
      v[len++] = space_code;
    } else {
      s = ss + 1;
      v[len++] = ov;
    }
  }

  *lenp = len;
  return v;
}

char *
labels_to_str(ofst_symbol_table_t *ost, int *v, int len)
{
  int i;
  int slen;
  char *s;
  char *ss;

  slen = 0;
  for (i = 0; i < len; i++) {
    char *sy = ofst_symbol_table_symbol(ost, v[i]);
    if (sy == NULL) {
      log_warning("%s: could not convert %d", __FUNCTION__, v[i]);
      return NULL;
    }
    slen += strlen(sy);
  }

  s = xmalloc(slen+1);
  ss = s;
  *ss = '\0'; // start from an empty string
  for (i = 0; i < len; i++) {
    ss = stpcpy(ss, ofst_symbol_table_symbol(ost, v[i]));
  }
  return s;
}
