#pragma once
#include "knn_index.h"

#include <alice/boltalka/libs/dssm_model/dssm_pool.h>

#include <util/folder/path.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NNlg {

class TDssmIndex : public TThrRefBase {
public:
    enum class ESearchBy {
        Context /* "context" */,
        Reply /* "reply" */,
        ContextAndReply /* "context_and_reply" */
    };

    enum class ESearchFor {
        Reply /* "reply" */,
        Context /* "context" */,
        ContextAndReply /* "context_and_reply" */,
        Score /* "score" */
    };

    struct TSearchOptions {
        size_t MaxResults = 1;
        double MinRatioWithBestResponse = 1.0;
        size_t KnnSearchNeighborhoodSize = 1400;
        size_t KnnDistanceCalcLimit = 35000;
        size_t KnnNumCandidates = 100;
        ESearchBy SearchBy = ESearchBy::Context;
        ESearchFor SearchFor = ESearchFor::Reply;
        float ContextWeight = 1.0;
        bool FallbackToSearchForReply = false;
    };

    TDssmIndex(TDssmPoolPtr dssmPool, const TFsPath& indexDir, size_t embeddingDimension, EMemoryMode memoryMode);
    TDssmIndex(TDssmPoolPtr dssmPool, const THashMap<ESearchFor, TKnnIndexPtr>& knnIndexes);
    TVector<TKnnIndex::TSearchResult> GetReplies(const TVector<TVector<float>>& queryEmbeddings, const TSearchOptions& options) const;

    TKnnIndexPtr GetKnnIndex(ESearchFor searchFor) const;

private:
    TDssmPoolPtr DssmPool;
    THashMap<ESearchFor, TKnnIndexPtr> KnnIndexes;
};

using TDssmIndexPtr = TIntrusivePtr<TDssmIndex>;

}
