/**
 * @file util/wfst.h
 *
 * @date 2014
 *
 * @author Georgy Bronnikov <gogabr@yandex-team.ru>
 *
 * @copyright 2014 Yandex. All rights reserved.
 *
 * Weighted Finite State Transducers; OpenFST's "const" format.
 */

#pragma once

#include "config.h"
#include "mem_blob.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wfst wfst_t;

typedef struct wfst_arc {
  int32_t ilabel;
  int32_t olabel;
  REAL weight;
  int32_t nextstate;
} wfst_arc_t;

wfst_t *wfst_read(char *fname);
wfst_t *wfst_read_from_blob(const char *fst_name, mem_blob_t fst);
void wfst_free(wfst_t *wfst);

char *wfst_fname(wfst_t *wfst);
int32_t wfst_num_states(wfst_t *wfst);
int32_t wfst_start_state(wfst_t *wfst);

// INFINITY when the state is not final.
REAL wfst_state_final_cost(wfst_t *wfst, int32_t state_id);
wfst_arc_t *wfst_state_arcs(wfst_t *wfst, int32_t state_id, int32_t *num_arcs_p);


typedef struct ofst_symbol_table ofst_symbol_table_t;

int *str_to_labels(ofst_symbol_table_t *sym, char *result, int *len_ptr);
char *labels_to_str(ofst_symbol_table_t *sym, int *result, int len);

#ifdef __cplusplus
}
#endif
