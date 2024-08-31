#include "dssm_index.h"

#include <library/cpp/dot_product/dot_product.h>

#include <kernel/searchlog/errorlog.h>

namespace NNlg {

TDssmIndex::TDssmIndex(TDssmPoolPtr dssmPool, const TFsPath& indexDir, size_t embeddingDimension, EMemoryMode memoryMode)
    : DssmPool(dssmPool)
{
    for (auto searchFor : { ESearchFor::Reply, ESearchFor::Context, ESearchFor::ContextAndReply }) {
        TString indexPrefix = indexDir / ToString(searchFor);
        if (!TFsPath(indexPrefix + ".index").Exists()) {
            continue;
        }
        size_t dimension = searchFor == ESearchFor::ContextAndReply ? embeddingDimension * 2 : embeddingDimension;
        KnnIndexes[searchFor] = new TKnnIndex(indexPrefix, dimension, memoryMode);
        SEARCH_INFO << "Loaded knn index: " << indexPrefix;
    }
}

TDssmIndex::TDssmIndex(TDssmPoolPtr dssmPool, const THashMap<ESearchFor, TKnnIndexPtr>& knnIndexes)
    : DssmPool(dssmPool)
    , KnnIndexes(knnIndexes)
{
}

TKnnIndexPtr TDssmIndex::GetKnnIndex(ESearchFor searchFor) const {
    auto it = KnnIndexes.find(searchFor);
    if (it == KnnIndexes.end()) {
        return nullptr;
    }
    return it->second;
}

TVector<TKnnIndex::TSearchResult> TDssmIndex::GetReplies(const TVector<TVector<float>>& queryEmbeddings,
                                                         const TSearchOptions& options) const {
    if (queryEmbeddings.empty()) {
        return {};
    }

    if (options.SearchFor == ESearchFor::Score) {
        Y_VERIFY(queryEmbeddings.size() == 2);
        size_t dimension = queryEmbeddings[0].size();
        float score = DotProduct(queryEmbeddings[0].data(), queryEmbeddings[1].data(), dimension);
        return { {Max<ui32>(), Max<ui32>(), score} };
    }

    TVector<float> vectorQuery;
    for (const auto& embedding : queryEmbeddings) {
        vectorQuery.insert(vectorQuery.end(), embedding.begin(), embedding.end());
    }

    auto searchFor = options.SearchFor;
    auto knnIndex = GetKnnIndex(searchFor);
    if (!knnIndex) {
        if (options.FallbackToSearchForReply && searchFor == ESearchFor::ContextAndReply) {
            searchFor = ESearchFor::Reply;
            knnIndex = GetKnnIndex(searchFor);
        }
        if (!knnIndex) {
            return {};
        }
    }

    if (searchFor == ESearchFor::ContextAndReply) {
        if (queryEmbeddings.size() == 1) {
            vectorQuery.insert(vectorQuery.end(), queryEmbeddings[0].begin(), queryEmbeddings[0].end());
        }
        for (size_t i = 0; i < queryEmbeddings[0].size(); ++i) {
            vectorQuery[i] *= options.ContextWeight;
        }
    }

    const size_t numCandidates = Max<size_t>(options.MaxResults, options.KnnNumCandidates);
    return knnIndex->FindNearestVectors(vectorQuery.data(),
                                        numCandidates,
                                        options.KnnSearchNeighborhoodSize,
                                        options.KnnDistanceCalcLimit);

}

}
