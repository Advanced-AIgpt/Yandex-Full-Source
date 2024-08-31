#include "dssm_model_with_indexes.h"
#include "util.h"

#include <kernel/searchlog/errorlog.h>
#include <library/cpp/iterator/enumerate.h>

namespace NNlg {

using TDssmSearchResult = TDssmModelWithIndexes::TSearchResult;

namespace {

THashMap<TString, TDssmIndexPtr> LoadEntityIndexes(TDssmPoolPtr dssmPool,
                                                            const TFsPath& indexDir,
                                                            const TString& entityType,
                                                            size_t embeddingDimension,
                                                            EMemoryMode memoryMode) {
    const TFsPath entityIndexDir = indexDir / entityType;
    TVector<TString> entityNames = StringSplitter(TFileInput(entityIndexDir / "entities").ReadAll()).Split('\n').SkipEmpty();
    for (auto& entityName: entityNames) {
        entityName = entityType + ":" + entityName;
    }
    THashMap<TDssmIndex::ESearchFor, TBlobWithOffsets> indexBlobs;
    THashMap<TDssmIndex::ESearchFor, TBlobWithOffsets> vectorBlobs;
    THashMap<TDssmIndex::ESearchFor, TBlobWithOffsets> idsBlobs;

    for (auto searchFor : { TDssmIndex::ESearchFor::Reply,
                            TDssmIndex::ESearchFor::Context,
                            TDssmIndex::ESearchFor::ContextAndReply }) {
        const TString indexPrefix = entityIndexDir / ToString(searchFor);
        if (!TFsPath(indexPrefix + ".index").Exists()) {
            continue;
        }
        const auto indexBlob = LoadBlobWithOffsets(indexPrefix + ".index", memoryMode);
        Y_ENSURE(indexBlob.Offsets.size() == entityNames.size() + 1);
        indexBlobs.emplace(searchFor, indexBlob);
        const auto vectorBlob = LoadBlobWithOffsets(indexPrefix + ".vec", memoryMode);
        Y_ENSURE(vectorBlob.Offsets.size() == entityNames.size() + 1);
        vectorBlobs.emplace(searchFor, vectorBlob);
        const auto idsBlob = LoadBlobWithOffsets(indexPrefix + ".ids", memoryMode);
        Y_ENSURE(idsBlob.Offsets.size() == entityNames.size() + 1);
        idsBlobs.emplace(searchFor, idsBlob);
    }

    THashMap<TString, TDssmIndexPtr> entityIndexes(entityNames.size());
    for (const auto& [i, entityName] : Enumerate(entityNames)) {
        THashMap<TDssmIndex::ESearchFor, TKnnIndexPtr> knnIndexes;
        for (auto searchFor : { TDssmIndex::ESearchFor::Reply,
                            TDssmIndex::ESearchFor::Context,
                            TDssmIndex::ESearchFor::ContextAndReply }) {
            const auto* indexBlob = indexBlobs.FindPtr(searchFor);
            if (!indexBlob) {
                continue;
            }
            const auto* vectorBlob = vectorBlobs.FindPtr(searchFor);
            const auto* idsBlob = idsBlobs.FindPtr(searchFor);
            const size_t dimension = searchFor == TDssmIndex::ESearchFor::ContextAndReply ? embeddingDimension * 2 : embeddingDimension;
            knnIndexes[searchFor] = new TKnnIndex(
                indexBlob->Data.SubBlob(indexBlob->Offsets[i], indexBlob->Offsets[i + 1]),
                vectorBlob->Data.SubBlob(vectorBlob->Offsets[i], vectorBlob->Offsets[i + 1]),
                idsBlob->Data.SubBlob(idsBlob->Offsets[i], idsBlob->Offsets[i + 1]),
                dimension);
        }
        entityIndexes[entityName] = new TDssmIndex(dssmPool, knnIndexes);
    }
    return entityIndexes;
}

TVector<TDssmSearchResult> MergeSortedReplies(const THashMap<TString, TVector<TDssmSearchResult>>& sortedReplies, size_t topSize,
        const TVector<TString>& priorityKnnIndexes) {
    if (sortedReplies.size() == 1) {
        return sortedReplies.begin()->second;
    }

    THashMap<TString, TVector<TDssmSearchResult>::const_iterator> sortedRepliesIterators;
    auto dssmSearchResultWorse = [](const TDssmSearchResult& a, const TDssmSearchResult& b) {
        return a.Score < b.Score;
    };
    TPriorityQueue<TDssmSearchResult, TVector<TDssmSearchResult>, decltype(dssmSearchResultWorse)> currentBestReplies(dssmSearchResultWorse);

    TVector<TDssmSearchResult> mergedReplies;
    mergedReplies.reserve(topSize);
    for (const auto& pair : sortedReplies) {
        const auto& knnIndexName = pair.first;
        if (FindPtr(priorityKnnIndexes, knnIndexName)) {
            std::copy(pair.second.cbegin(), pair.second.cend(), std::back_inserter(mergedReplies));
        } else {
            sortedRepliesIterators[knnIndexName] = sortedReplies.at(knnIndexName).cbegin();
            if (sortedRepliesIterators[knnIndexName] != sortedReplies.at(knnIndexName).cend()) {
                currentBestReplies.push(*(sortedRepliesIterators[knnIndexName]++));
            }
        }
    }
    while (mergedReplies.size() < topSize && !currentBestReplies.empty()) {
        auto currentBestReply = currentBestReplies.top();
        currentBestReplies.pop();
        const auto& knnIndexName = currentBestReply.KnnIndexName;
        mergedReplies.push_back(currentBestReply);
        if (sortedRepliesIterators[knnIndexName] != sortedReplies.at(knnIndexName).cend()) {
            currentBestReplies.push(*(sortedRepliesIterators[knnIndexName]++));
        }
    }
    mergedReplies.resize(Min(mergedReplies.size(), topSize));
    return mergedReplies;
}

}

TDssmModelWithIndexes::TDssmModelWithIndexes(const TFsPath& modelDir,
                                             const TVector<TString>& knnIndexNames,
                                             const TVector<TString>& entityIndexNames,
                                             size_t numSearchThreads,
                                             EMemoryMode memoryMode)
    : DssmPool(new TDssmPool(modelDir / "model", numSearchThreads))
    , Dimension(DssmPool->Fprop({""}, "", {"query_embedding"})[0].size()) // TODO(alipov): looks kinda hacky
    , ZeroEmbedding(Dimension * 2)
{
    SEARCH_INFO << "Loaded dssm model: " << modelDir;

    const size_t embeddingDimension = Dimension;
    SEARCH_INFO << "Embedding dimension is " << embeddingDimension << Endl;

    for (const auto& knnIndexName: knnIndexNames) {
        auto dssmIndexDir = modelDir / knnIndexName;
        if (!dssmIndexDir.Exists()) {
            continue;
        }
        DssmIndexes[knnIndexName] = new TDssmIndex(DssmPool, dssmIndexDir, embeddingDimension, memoryMode);
    }
    for (const auto& entityIndexName : entityIndexNames) {
        auto entityIndexes = LoadEntityIndexes(DssmPool, modelDir, entityIndexName, embeddingDimension, memoryMode);
        DssmIndexes.insert(entityIndexes.begin(), entityIndexes.end());
        SEARCH_INFO << "Loaded entity knn indexes: " << entityIndexName;
    }
}

TDssmIndexPtr TDssmModelWithIndexes::GetDssmIndex(const TString& knnIndexName) const {
    auto it = DssmIndexes.find(knnIndexName);
    if (it == DssmIndexes.end()) {
        return nullptr;
    }
    return it->second;
}

TVector<TVector<float>> TDssmModelWithIndexes::GetQueryEmbeddings(const TVector<TString>& turns,
                                                                  const TSearchOptions& options) const {
    TVector<TString> context = {""};
    TString reply;
    TVector<TString> outputVariables;
    switch (options.SearchBy) {
        case TDssmIndex::ESearchBy::Context: {
            if (!turns.empty()) {
                context = turns;
            }
            outputVariables = { "query_embedding" };
            break;
        };
        case TDssmIndex::ESearchBy::Reply: {
            if (!turns.empty()) {
                reply = turns.back();
            }
            outputVariables = { "doc_embedding" };
            break;
        };
        case TDssmIndex::ESearchBy::ContextAndReply: {
            if (turns.size() < 2) {
                return {};
            }
            context = turns;
            context.pop_back();
            reply = turns.back();
            outputVariables = { "query_embedding", "doc_embedding" };
            break;
        };
    }

    TVector<TVector<float>> embeddings = DssmPool->Fprop(context, reply, outputVariables);

    Y_VERIFY(embeddings.size() == 1 || embeddings.size() == 2 && embeddings[0].size() == embeddings[1].size());
    return embeddings;
}

TVector<float> TDssmModelWithIndexes::GetReplyEmbedding(const TString& reply) const {
    TVector<TString> context = {""};
    TVector<float> embedding = DssmPool->Fprop(context, reply, {"doc_embedding"})[0];

    return embedding;
}

TVector<TVector<float>> TDssmModelWithIndexes::GetReplyEmbeddings(const TVector<TString>& replies) const {
    TVector<TVector<TString>> contexts(replies.size(), {""});
    auto data = DssmPool->FpropBatch(contexts, replies, {"doc_embedding"})[0];
    TVector<TVector<float>> embeddings;
    embeddings.reserve(replies.size());
    for (size_t i = 0; i < data.size(); i += Dimension) {
        embeddings.emplace_back(data.begin() + i, data.begin() + i + Dimension);
    }
    return embeddings;
}

TDssmIndex::TSearchOptions TDssmModelWithIndexes::OverrideKnnIndexSpecificOptions(TSearchOptions options, const TString& knnIndexName) const {
    const auto entityIndexName = DropEntityIdFromIndexName(knnIndexName);
    if (auto* indexOptions = options.KnnOptions.FindPtr(entityIndexName)) {
        options.KnnSearchNeighborhoodSize = indexOptions->KnnSearchNeighborhoodSize;
        options.KnnDistanceCalcLimit = indexOptions->KnnDistanceCalcLimit;
        options.KnnNumCandidates = indexOptions->KnnNumCandidates;
        options.MaxResults = indexOptions->MaxResults;
    }
    if (IsEntityIndex(knnIndexName)) {
        options.FallbackToSearchForReply = true;
        if (options.EntityIndexNumCandidates > 0) {
            options.KnnNumCandidates = options.EntityIndexNumCandidates;
            options.MaxResults = options.EntityIndexNumCandidates;
        }
    }
    return options;
}

TVector<TDssmSearchResult> TDssmModelWithIndexes::GetReplies(const TVector<TVector<float>>& queryEmbeddings,
                                                             const TVector<TString>& knnIndexNamesAllowed,
                                                             const TSearchOptions& options,
                                                             const TVector<TString>& priorityKnnIndexes) const {
    THashMap<TString, TVector<TDssmSearchResult>> replies;
    for (const auto& knnIndexName : knnIndexNamesAllowed) {
        auto dssmIndex = GetDssmIndex(knnIndexName);
        if (!dssmIndex) {
            continue;
        }
        for (const auto& knnReply : dssmIndex->GetReplies(queryEmbeddings, OverrideKnnIndexSpecificOptions(options, knnIndexName))) {
            replies[knnIndexName].emplace_back(knnReply, knnIndexName);
        }
    }
    const size_t numCandidates = Max<size_t>(options.MaxResults, options.KnnNumCandidates);
    return MergeSortedReplies(replies, numCandidates, priorityKnnIndexes);
}

const float* TDssmModelWithIndexes::GetZeroEmbedding() const {
    return ZeroEmbedding.data();
}

}
