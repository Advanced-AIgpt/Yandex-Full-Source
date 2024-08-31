#pragma once

#include "token.h"

struct TAllocatorImpl;
typedef struct TTokenAllocator {
    struct TAllocatorImpl* Impl;
} TTokenAllocator;

#ifdef __cplusplus
extern "C" {
#endif

TTokenAllocator CreateAllocator();
TTokenAllocator GetOrCreateThreadLocalAllocator();
void DestroyAllocator(TTokenAllocator* allocator);
void ClearAllocator(TTokenAllocator* allocator);

token_t* AllocToken(TTokenAllocator* allocator);
void FreeToken(TTokenAllocator* allocator, token_t* token);

#ifdef __cplusplus
}
#endif
