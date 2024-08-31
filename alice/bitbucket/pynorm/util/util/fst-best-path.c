/**
 * @file util/fst-best-path.c
 *
 * @date 2015.02.03
 *
 * @author Georgy Bronnikov <gogabr@yandex-team.ru>
 *
 * @copyright 2015 Yandex. All rights reserved.
 *
 * Perform a Viterbi best path search on an FST.
 * Used in reverse normalizer.
 * The code is a much simplified version of decode-task.c;
 * written for clarity, not for speed.
 */

#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h> // only needed for wfst.h, cuckoo-hash.h

#include "config.h"
#include "util.h"
#include "log.h"
#include "wfst.h"
#include "cuckoo-hash.h"
#include "xalloc.h"

#include "fst-best-path.h"
#include "sparse_vector.h"
#include "token_allocator.h"

static token_t *make_token(int state, TTokenAllocator* alloc);
static bool possibly_update_token(token_t *tok, token_t *prev, wfst_arc_t *arc, REAL score, TTokenAllocator* alloc); // returns true when updated
static void token_unlink(token_t *tok, TTokenAllocator* alloc);
static void tokens_array_unlink(int ntoks, token_t **toks, TTokenAllocator* alloc);
static int *token_get_path(token_t *tok, int *pathlen);

static token_t** make_start_state(wfst_t* fst, int* ntoks, TTokenSparseVector* tok_set, TTokenAllocator* alloc);
static token_t** handle_step(wfst_t* fst, int ntoks, token_t** toks, int ilabel, int* new_ntoks,
                             TTokenSparseVector* tok_set, TTokenAllocator* alloc);
static token_t **prune_toks(int ntoks, token_t **toks, int band, int *new_ntoks, TTokenAllocator* alloc);
static token_t *get_best_tok(wfst_t *fst, int ntoks, token_t **toks);

static void handle_arcs(wfst_t* fst, TTokenSparseVector* tok_set, token_t* tok, int ilabel, TTokenAllocator* alloc);

static int
int_compare(void const *a, void const *b)
{
  int *ta = (int *)a; // MSVC is unhappy without explicit conversion.
  int *tb = (int *)b;

  if (*ta < *tb) {
    return -1;
  } else if (*ta > *tb) {
    return 1;
  } else {
    return 0;
  }
}

static token_t**
token_set_t_get_tokens(TTokenSparseVector* tok_set, int *szp)
{
    TTokenVector vec = GetAndUnmapTokens(tok_set);
    *szp = vec.Size;
    return vec.Tokens;
}

token_t* token_set_find_or_insert(TTokenSparseVector* tok_set, int state, TTokenAllocator* alloc) {
    token_t** found = FindTokenPtr(tok_set, state);
    if (found) {
        return *found;
    }
    token_t* new_tok = make_token(state, alloc);
    AddMapping(tok_set, new_tok);
    return new_tok;
}

/* Find best path given indata. Consider at most band variants.
   The returned value should be freed by free.

   On failure, NULL is returned and *outsize is set to a negative number.
*/
int *
fst_best_path_search(wfst_t *fst, int insize, int *indata, int band, int *outsize)
{
  int stepno;
  int ntoks;
  token_t **toks;

  ///**/log_info("***(***");

  TTokenAllocator alloc = GetOrCreateThreadLocalAllocator();
  TTokenSparseVector tok_set = CreateSparseVector();

  toks = make_start_state(fst, &ntoks, &tok_set, &alloc);
  ///**/log_info("ntoks[0] = %d", ntoks);

  for (stepno = 0; stepno < insize; stepno++) {
    int new_ntoks;
    token_t **newtoks;

    newtoks = handle_step(fst, ntoks, toks, indata[stepno], &new_ntoks, &tok_set, &alloc);
    ///**/log_info("ntoks[%d] = %d", stepno+1, new_ntoks);

    if (newtoks == NULL) {
      // NOTE (a-sidorin@): We don't need to unlink the tokens since the allocator is going to be free'd anyway.
      *outsize = -1;

      ClearAllocator(&alloc);
      DestroySparseVector(&tok_set);
      FinalizeFst();
      return NULL;
    }

    tokens_array_unlink(ntoks, toks, &alloc);
    toks = prune_toks(new_ntoks, newtoks, band, &ntoks, &alloc);
  }
  ///**/log_info("***)***");

  {
    token_t *best_tok;
    int *best_path;

    best_tok = get_best_tok(fst, ntoks, toks);
    if (best_tok == NULL) {
      /* there are live tokens, but none of them final */
      *outsize = -1;
      best_path = NULL;
    } else {
      /* The normal case */
      best_path = token_get_path(best_tok, outsize);
    }
    // NOTE (a-sidorin@): We don't need to unlink the tokens since the allocator is going to be free'd anyway.
    ClearAllocator(&alloc);
    DestroySparseVector(&tok_set);
    FinalizeFst();

    return best_path; // *outsize set in token_get_path.
  }
}

static token_t *
make_token(int state, TTokenAllocator* alloc)
{
  token_t *tok = AllocToken(alloc);
  tok->refcount = 1;
  tok->prev = NULL;
  tok->state = state;
  tok->arc = NULL;
  tok->score = (REAL)INFINITY;

  return tok;
}

/* Returns true iff updated */
static bool
possibly_update_token(token_t *tok, token_t *prev, wfst_arc_t *arc, REAL score, TTokenAllocator* alloc)
{
  if (score < tok->score) {
    token_unlink(tok->prev, alloc);
    if (prev != NULL) prev->refcount++;
    tok->prev = prev;

    tok->arc = arc;
    tok->score = score;

    return true;
  } else {
    return false;
  }
}

static void
token_unlink(token_t *tok, TTokenAllocator* alloc)
{
  /* Avoid recursion; token chains can be long */
  token_t *t = tok;
  while (t != NULL) {
    token_t *prev = t->prev;

    t->refcount--;
    if (t->refcount != 0) return;

    FreeToken(alloc, t);
    t = prev;
  }
}

static void
tokens_array_unlink(int ntoks, token_t **toks, TTokenAllocator *alloc)
{
  int i;
  for (i = 0; i < ntoks; i++) {
    token_unlink(toks[i], alloc);
  }
}

/* Get the path of nonzero olabel from the arcs leading to tok.
   Result to be freed by free().
 */
static int *
token_get_path(token_t *tok, int *pathlen)
{
  int *v;
  int ix;
  int len;
  token_t *t;

  len = 0;
  for (t = tok; t != NULL; t = t->prev) {
    /* At the last step, we haven't yet checked prev and arc is already NULL,
       so we have to check for that particular case.
     */
    if (t->arc != NULL && t->arc->olabel != 0) {
      len++;
    }
  }

  v = xmalloc(len * sizeof(int));
  ix = len;
  for (t = tok; t != NULL; t = t->prev) {
    if (t->arc != NULL && t->arc->olabel != 0) {
      v[--ix] = t->arc->olabel;
    }
  }
  assert(ix == 0);

  *pathlen = len;
  return v;
}

static token_t **
make_start_state(wfst_t *fst, int *ntoks, TTokenSparseVector* tok_set, TTokenAllocator* alloc)
{
  int start_state;
  token_t *start_tok;
  token_t **toks;

  start_state = wfst_start_state(fst);
  start_tok = make_token(start_state, alloc);
  possibly_update_token(start_tok, NULL, NULL, 0.0f, alloc);

  ClearMappings(tok_set);
  AddMapping(tok_set, start_tok);

  handle_arcs(fst, tok_set, start_tok, 0, alloc); // handle epsilons

  toks = token_set_t_get_tokens(tok_set, ntoks);
  return toks;
}

static token_t** handle_step(wfst_t* fst, int ntoks, token_t** toks, int ilabel, int* new_ntoks,
                             TTokenSparseVector* tok_set, TTokenAllocator* alloc) {
  token_t **new_toks;
  int i;

  ClearMappings(tok_set);
  for (i = 0; i < ntoks; i++) {
    handle_arcs(fst, tok_set, toks[i], ilabel, alloc);
  }

  new_toks = token_set_t_get_tokens(tok_set, new_ntoks);
  return new_toks;
}

static int
token_compare(void const *a, void const *b)
{
  token_t * const *ta = (token_t *const *)a; /* MSVC is unhappy without explicit conversion. */
  token_t * const *tb = (token_t *const *)b;

  if ((*ta)->score < (*tb)->score) {
    return -1;
  } else if ((*ta)->score > (*tb)->score) {
    return 1;
  } else {
    return 0;
  }
}

/* !!!!! Modifies toks */
static token_t **
prune_toks(int ntoks, token_t **toks, int band, int *new_ntoks, TTokenAllocator* alloc)
{
  if (ntoks > band) {
    qsort(toks, ntoks, sizeof(token_t*), token_compare);
    tokens_array_unlink(ntoks-band, toks + band, alloc);
    *new_ntoks = band;
    return toks;
  } else {
    *new_ntoks = ntoks;
    return toks;
  }
}

static token_t *
get_best_tok(wfst_t *fst, int ntoks, token_t **toks)
{
  int i;
  token_t *best_tok = NULL;
  REAL best_final_score = (REAL)INFINITY;

  for (i = 0; i < ntoks; i++) {
    REAL tok_final_score = toks[i]->score + wfst_state_final_cost(fst, toks[i]->state);
    if (tok_final_score < best_final_score) {
      best_tok = toks[i];
      best_final_score = tok_final_score;
    }
  }

  return best_tok;
}
static inline wfst_arc_t* lower_bound(wfst_arc_t* start, wfst_arc_t* finish, int label) {
  int len = finish - start;
  while (len > 0) {
    int half = len >> 1;
    wfst_arc_t* middle = start + half;
    if (middle->ilabel < label) {
      start = middle + 1;
      len -= half + 1;
    } else {
      len = half;
    }
  }
  return start;
}

static void handle_arcs(wfst_t* fst, TTokenSparseVector* tok_set, token_t* tok, int ilabel, TTokenAllocator* alloc) {
  int32_t narcs;
  wfst_arc_t* arcs = wfst_state_arcs(fst, tok->state, &narcs);

  int maxstate = 0;
  wfst_arc_t* a = ilabel == 0 ? arcs : lower_bound(arcs, arcs + narcs, ilabel);
  for (; (a < arcs + narcs) && (a->ilabel == ilabel); a++) {
      int nextstate = a->nextstate;
      if (nextstate > maxstate)
        maxstate = nextstate;
      token_t *nstok;
      nstok = token_set_find_or_insert(tok_set, nextstate, alloc);
      if (possibly_update_token(nstok, tok, a, tok->score + a->weight, alloc)) {
        handle_arcs(fst, tok_set, nstok, 0, alloc);
      }
  }
}
