#include "embedder.h"
#include <util/generic/yexception.h>

namespace NAlice {

TEmbedding TTokenEmbedder::EmbedToken(const TString& token, const TMaybe<TArrayRef<const float>>& defaultEmbedding) const {
    Y_ENSURE(defaultEmbedding.Empty() || defaultEmbedding->size() == static_cast<size_t>(EmbeddingsDimension),
             "Default embedding has improper dimension.");

    TTokenIndex tokenIndex;
    if (TokenToIndex.Find(token, &tokenIndex)) {
        const TArrayRef<const float> embeddingDataRef = GetTokenEmbeddingByIndex(tokenIndex);
        return {embeddingDataRef.begin(), embeddingDataRef.end()};
    }

    if (defaultEmbedding.Defined()) {
        return {defaultEmbedding->begin(), defaultEmbedding->end()};
    }

    return TEmbedding(EmbeddingsDimension, 0.0f);
}

TVector<TEmbedding> TTokenEmbedder::EmbedSequence(const TVector<TString>& tokens,
                                                  const TMaybe<TArrayRef<const float>>& defaultEmbedding) const {
    TVector<TEmbedding> tokenEmbeddings(Reserve(tokens.size()));
    for (auto&& token : tokens) {
        tokenEmbeddings.emplace_back(EmbedToken(token, defaultEmbedding));
    }
    return tokenEmbeddings;
}

TArrayRef<const float> TTokenEmbedder::GetTokenEmbeddingByIndex(TTokenIndex tokenIndex) const {
    auto shift = tokenIndex * EmbeddingsDimension;
    const float* embeddingBeginPtr = reinterpret_cast<const float*>(Embeddings.AsCharPtr()) + shift;
    return TArrayRef<const float>(embeddingBeginPtr, embeddingBeginPtr + EmbeddingsDimension);
}

} // namespace NAlice
