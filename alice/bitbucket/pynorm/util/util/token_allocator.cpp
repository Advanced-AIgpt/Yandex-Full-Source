#include "token_allocator.h"
#include "allocator.h"

#include <util/thread/singleton.h>

// TTokenAllocator -------------------------------------------------------------
struct TAllocatorImpl {
    // NOTE (a-sidorin, ALICEINFRA-68): q90 for storage usage is 78 elements, q99 is 354.
    static constexpr size_t SMALL_SIZE = 512;
    TSmallTypedAllocator<token_t, SMALL_SIZE> Storage;
};

TTokenAllocator CreateAllocator() {
    auto* ptr = new TAllocatorImpl;
    return {ptr};
}

TTokenAllocator GetOrCreateThreadLocalAllocator() {
    return {FastTlsSingleton<TAllocatorImpl>()};
}

void DestroyAllocator(TTokenAllocator* allocator) {
    delete allocator->Impl;
}

void ClearAllocator(TTokenAllocator* allocator) {
    allocator->Impl->Storage.Clear();
}

token_t* AllocToken(TTokenAllocator* allocator) {
    return &allocator->Impl->Storage.Alloc();
}

void FreeToken(TTokenAllocator* allocator, token_t* token) {
    allocator->Impl->Storage.Free(*token);
}
