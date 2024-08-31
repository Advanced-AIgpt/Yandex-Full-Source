#include "external_info.h"

#include <string>


namespace NGenerativeBoltalka {
    TDocsStorage::TDocsStorage(const TString& docsPath) {
        TFileInput input(docsPath);
        const auto data = NJson::ReadJsonTree(&input, /* throwOnError = */ true).GetArray();

        Docs.reserve(data.size());

        for (size_t i = 0; i < data.size(); ++i) {
            Docs.emplace_back(data[i].GetString());
        }
    }

    const TString& TDocsStorage::GetDoc(uint32_t id) const {
        return Docs[id];
    }

    size_t TDocsStorage::GetTotalNumber() const {
        return Docs.size();
    }

    THnswRetriever::THnswRetriever(const TParams& params)
        : Docs(params.DocsPath)
        , IdsData(TBlob::LockedFromFile(params.PathPrefix + ".ids"))
        , SearchNeighborhoodSize(params.SearchNeighborhoodSize)
        , DistanceCalcLimit(params.DistanceCalcLimit) {
            const auto vecBlob = TBlob::LockedFromFile(params.PathPrefix + ".vec");
            int64_t embeddingSize = vecBlob.Length() / Docs.GetTotalNumber() / sizeof(float);

            KnnSearcherPtr = std::make_unique<NHnsw::THnswDenseVectorIndex<float>>(
                TBlob::LockedFromFile(params.PathPrefix + ".index"),
                vecBlob,
                embeddingSize
            );
        }

    IEmbeddingRetriever::RetrieverResult THnswRetriever::Retrieve(const std::vector<float>& embedding, size_t topK) {
        Y_ENSURE(topK > 0, "Top K documents must be positive, found: " << topK);

        IEmbeddingRetriever::RetrieverResult results;
        results.Docs.reserve(topK);
        results.Ids.reserve(topK);

        using TDistance = NHnsw::TDotProduct<float>;

        auto bestMatches = KnnSearcherPtr->GetNearestNeighbors<TDistance>(embedding.data(), topK, SearchNeighborhoodSize, DistanceCalcLimit);
        const uint32_t* ids = reinterpret_cast<const uint32_t*>(IdsData.Begin());

        for (const auto& match : bestMatches) {
            const uint32_t ind = ids[match.Id];
            TRetrieverDocument doc {Docs.GetDoc(ind), match.Dist};
            results.Docs.emplace_back(doc);
            results.Ids.emplace_back(ind);
        }

        return results;
    }

    TExternalInfoGenerativeBoltalka::TExternalInfoGenerativeBoltalka(const TExternalInfoGenerativeBoltalka::TParams& params)
        : Params(params)
        , Embedder(new TGenerativeBoltalka(params.EmbedderParams))
        , Retriever(new THnswRetriever(params.RetrieverParams))
        , Generator(new TGenerativeBoltalka(params.GeneratorParams))
    {}

    TVector<TGenerativeRequest> TExternalInfoGenerativeBoltalka::EnrichRequests(
        const TVector<TGenerativeRequest>& requestsBatch,
        const TVector<TVector<TRetrieverDocument>>& retrievedDocs) const {
        Y_ENSURE(requestsBatch.size() == retrievedDocs.size());

        TVector<TGenerativeRequest> results;
        results.reserve(requestsBatch.size() * Params.NumTopDocs);
        for (size_t i = 0; i < requestsBatch.size(); i++) {
            for (const auto& doc : retrievedDocs[i]) {
                auto copyRequest = requestsBatch[i];
                copyRequest.Context.emplace_back(doc.Data);
                copyRequest.NumHypos = Params.NumHypsFromDoc;
                results.emplace_back(copyRequest);
            }
        }
        return results;
    }

    TVector<TVector<TGenerativeResponse>> TExternalInfoGenerativeBoltalka::GenerateResponses(const TVector<TGenerativeRequest>& requestsBatch) const {
        std::vector<std::vector<float>> embeddings;
        const size_t batchSize = requestsBatch.size();
        embeddings.reserve(batchSize);
        for (const auto& request : requestsBatch) {
            embeddings.emplace_back(Embedder->GenerateEmbed(request.Context, request.PtuneEmbeddings, nullptr, true, true, true));
        }

        TVector<TVector<TRetrieverDocument>> retrievedDocs;
        retrievedDocs.reserve(batchSize);
        for (const auto& embedding : embeddings) {
            retrievedDocs.emplace_back(
                Retriever->Retrieve(embedding, Params.NumTopDocs).Docs
            );
        }

        return GetMergedResponsesFromDifferentDocs(
            Generator->GenerateResponses(
                EnrichRequests(requestsBatch, retrievedDocs)
            ),
            retrievedDocs
        );
    }

    TVector<TVector<TGenerativeResponse>>
    TExternalInfoGenerativeBoltalka::GetMergedResponsesFromDifferentDocs(
        const TVector<TVector<TGenerativeResponse>>& augmentedResponses,
        const TVector<TVector<TRetrieverDocument>>& retrievedDocs) const {
        TVector<TVector<TGenerativeResponse>> responses(augmentedResponses.size() / Params.NumTopDocs);
        for (size_t i = 0; i < responses.size(); ++i) {
            for (size_t j1 = 0; j1 < Params.NumTopDocs; ++j1) {
                for (size_t j2 = 0; j2 < augmentedResponses[i * Params.NumTopDocs + j1].size(); ++j2) {
                    auto response = std::move(augmentedResponses[i * Params.NumTopDocs + j1][j2]);
                    response.ExternalInfo = retrievedDocs[i][j1].Data;
                    responses[i].emplace_back(response);
                }
            }
        }
        return responses;
    }

    TVector<TGenerativeResponse> TExternalInfoGenerativeBoltalka::GenerateResponses(const TGenerativeRequest& request) const {
        const auto result = GenerateResponses(TVector{request});
        Y_ENSURE(!result.empty());
        return result.front();
    }

    bool TExternalInfoGenerativeBoltalka::CheckRequestHasBadWords(const TGenerativeRequest& request) const {
        return Generator->CheckRequestHasBadWords(request);
    }
}
