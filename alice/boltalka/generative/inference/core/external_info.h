#pragma once

#include "data.h"
#include "model.h"

#include <random>
#include <library/cpp/hnsw/index/dense_vector_index.h>
#include <library/cpp/json/json_reader.h>
#include <util/generic/ptr.h>
#include <util/stream/file.h>


namespace NGenerativeBoltalka {
    class TDocsStorage {
    public:
        TDocsStorage(const TString& docsPath);
        const TString& GetDoc(uint32_t id) const;
        size_t GetTotalNumber() const;

    private:
        TVector<TString> Docs;
    };

    struct TRetrieverDocument {
        TString Data;
        float Distance;
    };

    class IEmbeddingRetriever : public TThrRefBase {
    public:
        using TPtr = TIntrusivePtr<IEmbeddingRetriever>;

        struct RetrieverResult {
            TVector<TRetrieverDocument> Docs;
            TVector<uint32_t> Ids;
        };

    public:
        virtual RetrieverResult Retrieve(const std::vector<float>& embedding, size_t topK) = 0;
    };

    class THnswRetriever : public IEmbeddingRetriever {
    public:
        using TIndexPtr = std::unique_ptr<NHnsw::THnswDenseVectorIndex<float>>;

        struct TParams {
            TString PathPrefix;
            TString DocsPath;
            uint32_t SearchNeighborhoodSize;
            uint32_t DistanceCalcLimit;
        };

        THnswRetriever(const TParams& params);
        IEmbeddingRetriever::RetrieverResult Retrieve(const std::vector<float>& embedding, size_t topK) override;

    private:
        TDocsStorage Docs;
        TIndexPtr KnnSearcherPtr;
        TBlob IdsData;
        uint32_t SearchNeighborhoodSize;
        uint32_t DistanceCalcLimit;        
    };

    class TExternalInfoGenerativeBoltalka : public TThrRefBase {
    public:
        struct TParams {
            TGenerativeBoltalka::TParams EmbedderParams;
            THnswRetriever::TParams RetrieverParams;
            TGenerativeBoltalka::TParams GeneratorParams;

            uint32_t NumTopDocs = 1;
            uint32_t NumHypsFromDoc = 1;
        };

    public:
        TExternalInfoGenerativeBoltalka(const TParams& Params);
        TVector<TGenerativeResponse> GenerateResponses(const TGenerativeRequest& request) const;
        TVector<TVector<TGenerativeResponse>> GenerateResponses(const TVector<TGenerativeRequest>& requestsBatch) const;
        bool CheckRequestHasBadWords(const TGenerativeRequest& request) const;

    private:
        TVector<TGenerativeRequest> EnrichRequests(
            const TVector<TGenerativeRequest>& requestsBatch,
            const TVector<TVector<TRetrieverDocument>>& retrievedDocs) const;

        TVector<TVector<TGenerativeResponse>> GetMergedResponsesFromDifferentDocs(
            const TVector<TVector<TGenerativeResponse>>& augmentedResponses,
            const TVector<TVector<TRetrieverDocument>>& retrievedDocs) const;
        
    private:
        TParams Params;
        TGenerativeBoltalka::TPtr Embedder;
        IEmbeddingRetriever::TPtr Retriever;
        TGenerativeBoltalka::TPtr Generator;
    };
} // namespace NGenerativeBoltalka
