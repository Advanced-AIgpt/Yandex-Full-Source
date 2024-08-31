#pragma once

/**
 * @file util/fst-best-path.h
 *
 * @date 2015.02.03
 *
 * @author Georgy Bronnikov <gogabr@yandex-team.ru>
 *
 * @copyright 2015 Yandex. All rights reserved.
 *
 * Perform a Viterbi best path search on an FST.
 * Used in reverse normalizer.
 */

#include "wfst.h"

/* Find best path given indata. Consider at most band variants.
   The returned value should be freed by free.

   On failure, NULL is returned and *outsize is set to a negative number.
*/
int *fst_best_path_search(wfst_t *fst, int insize, int *indata, int band, int *outsize);
