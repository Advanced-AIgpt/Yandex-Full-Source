#pragma once

#include "config.h"
#include "wfst.h"

#include <stdint.h>

typedef struct token {
  wfst_arc_t *arc;
  struct token *prev;
  int state;
  REAL score;
  uint32_t refcount;
  uint32_t index;
} token_t;
