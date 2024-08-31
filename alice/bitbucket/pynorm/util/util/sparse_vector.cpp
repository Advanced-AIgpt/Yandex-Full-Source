#include "sparse_vector.h"

#include "small_stack.h"

#include <library/cpp/containers/dense_hash/dense_hash.h>

#include <util/thread/singleton.h>

namespace  {

// NOTE (a-sidorin, ALICEINFRA-68): q90 for storage usage is 78 elements, q99 is 354.
// But without 'remove_space_at_start' normalizer the size is usually very small (less than 10).
constexpr size_t SMALL_SIZE = 32;

using TTempTokenVector = TVector<token_t*>;
using TIndexVector = TVector<int>;

struct TTempVectors {
    TTempTokenVector& GetFreeVector() {
        auto& ret = Vectors[ui8{FreeIdx}];
        FreeIdx = !FreeIdx;
        return ret;
    }

    void Clear() {
        Vectors[0].clear();
        Vectors[1].clear();
    }

    TTempTokenVector Vectors[2];
    bool FreeIdx = false;
};

TTempVectors& GetOrCreateTempTokenVectors() {
    return *FastTlsSingleton<TTempVectors>();
}

TIndexVector& GetOrCreateIndexVector() {
    return *FastTlsSingleton<TIndexVector>();
}

} // namespace

// TTokenSparseVector ----------------------------------------------------------
struct TSparseVectorImpl {
    static constexpr auto EMPTY_KEY = SIZE_MAX;

    TSparseVectorImpl()
        : Mapping{EMPTY_KEY, SMALL_SIZE}
        , Indices(GetOrCreateIndexVector())
    {
    }

    TDenseHash<int, token_t*> Mapping;
    TIndexVector& Indices;
};

TTokenSparseVector CreateSparseVector() {
    auto* ptr = new TSparseVectorImpl;
    return {ptr};
}

void DestroySparseVector(TTokenSparseVector* vec) {
    delete vec->Impl;
    vec->Impl = nullptr;
}

void AddMapping(TTokenSparseVector* vec, token_t* token) {
    Y_ASSERT(!FindTokenPtr(vec, token->state));
    vec->Impl->Mapping[token->state] = token;
    vec->Impl->Indices.emplace_back(token->state);
}

void ClearMappings(TTokenSparseVector* vec) {
    vec->Impl->Indices.clear();
    vec->Impl->Mapping.Clear();
}

token_t** FindTokenPtr(TTokenSparseVector* vec, int state) {
    return MapFindPtr(vec->Impl->Mapping, state);
}

TTokenVector GetAndUnmapTokens(TTokenSparseVector* vec) {
    auto size = vec->Impl->Mapping.Size();
    if (!size) {
        return {nullptr, 0};
    }

    auto& indices = vec->Impl->Indices;
    Sort(indices);

    auto& result = GetOrCreateTempTokenVectors().GetFreeVector();
    result.resize(size);
    for (size_t i = 0, e = indices.size(); i < e; ++i) {
        token_t** token = vec->Impl->Mapping.FindPtr(indices[i]);
        Y_ASSERT(token);
        result[i] = *token;
    }

    ClearMappings(vec);
    return {result.data(), size};
}

void FinalizeFst() {
    GetOrCreateTempTokenVectors().Clear();
    GetOrCreateIndexVector().clear();
}
