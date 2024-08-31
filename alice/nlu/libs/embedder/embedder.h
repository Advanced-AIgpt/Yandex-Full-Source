#pragma once

#include <library/cpp/containers/comptrie/comptrie.h>
#include <util/generic/array_ref.h>
#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/memory/blob.h>

namespace NAlice {
    // TODO(dan-anastasev): actually, ui64 may be more safe
    using TTokenIndex = ui32;
    using TEmbedding = TVector<float>;

    constexpr size_t TEmbeddingElementSize = sizeof(float);

    class ITokenEmbedder {
    public:
        virtual ~ITokenEmbedder() = default;
        virtual TEmbedding EmbedToken(const TString& token, const TMaybe<TArrayRef<const float>>& defaultEmbedding = Nothing()) const = 0;
    };

    class TTokenEmbedder : public ITokenEmbedder {
    public:
        TTokenEmbedder(const TBlob& embeddings, const TBlob& tokenToIndexTrie, int embeddingsDimension = 300)
            : Embeddings(embeddings)
            , TokenToIndex(tokenToIndexTrie)
            , EmbeddingsDimension(embeddingsDimension)
        { }

        TEmbedding EmbedToken(const TString& token, const TMaybe<TArrayRef<const float>>& defaultEmbedding = Nothing()) const override;

        TVector<TEmbedding> EmbedSequence(const TVector<TString>& tokens,
                                          const TMaybe<TArrayRef<const float>>& defaultEmbedding = Nothing()) const;

    private:
        TArrayRef<const float> GetTokenEmbeddingByIndex(TTokenIndex tokenIndex) const;

    private:
        TBlob Embeddings;
        TCompactTrie<char, TTokenIndex> TokenToIndex;
        int EmbeddingsDimension;
    };

} // NBg
