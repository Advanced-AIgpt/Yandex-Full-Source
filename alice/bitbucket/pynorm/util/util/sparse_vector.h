#pragma once

#include "token.h"

struct TSparseVectorImpl;
typedef struct TokenSparseVector {
    struct TSparseVectorImpl* Impl;
} TTokenSparseVector;

#ifdef __cplusplus
extern "C" {
#endif

TTokenSparseVector CreateSparseVector();
TTokenSparseVector GetOrCreateThreadLocalSparseVector();
void DestroySparseVector(TTokenSparseVector* vec);

void AddMapping(TTokenSparseVector* vec, token_t* token);
void ClearMappings(TTokenSparseVector* vec);

token_t** FindTokenPtr(TTokenSparseVector* vec, int state);

typedef struct {
    token_t** Tokens;
    size_t Size;
} TTokenVector;

TTokenVector GetAndUnmapTokens(TTokenSparseVector* vec);

void FinalizeFst();

#ifdef __cplusplus
}
#endif
