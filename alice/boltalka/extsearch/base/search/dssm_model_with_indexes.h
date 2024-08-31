#pragma once
#include "dssm_index.h"

#include <util/string/cast.h>

namespace NNlg {

struct TListOption : TVector<TString> {
    using TVector<TString>::TVector;
};

class TDssmModelWithIndexes : public TThrRefBase {
public:
    struct TSearchOptions : TDssmIndex::TSearchOptions {
        TString BaseKnnIndexName;
        TListOption KnnIndexNames;
        TListOption EntityIndexNames;
        THashMap<TString, TDssmIndex::TSearchOptions> KnnOptions;
        ui64 EntityIndexNumCandidates = 0;
    };
    struct TSearchResult : TKnnIndex::TSearchResult {
        TString KnnIndexName;
        TSearchResult() = default;
        TSearchResult(const TKnnIndex::TSearchResult& knnSearchResult, const TString& knnIndexName)
            : TKnnIndex::TSearchResult(knnSearchResult)
            , KnnIndexName(knnIndexName)
        {
        }
    };
    TDssmModelWithIndexes(const TFsPath& modelDir, const TVector<TString>& knnIndexNames, const TVector<TString>& entityIndexNames, size_t numSearchThreads, EMemoryMode memoryMode);
    TVector<TVector<float>> GetQueryEmbeddings(const TVector<TString>& turns, const TSearchOptions& options) const;
    TVector<float> GetReplyEmbedding(const TString& reply) const;
    TVector<TVector<float>> GetReplyEmbeddings(const TVector<TString>& replies) const;
    TVector<TSearchResult> GetReplies(const TVector<TVector<float>>& queryEmbeddings, const TVector<TString>& knnIndexNamesAllowed,
            const TSearchOptions& options, const TVector<TString>& priorityKnnIndexes = {}) const;
    TDssmIndex::TSearchOptions OverrideKnnIndexSpecificOptions(TSearchOptions options, const TString& knnIndexName) const;

    TDssmIndexPtr GetDssmIndex(const TString& knnIndexName) const;
    const float* GetZeroEmbedding() const;
    size_t GetDimension() const {
        return Dimension;
    }

private:
    TDssmPoolPtr DssmPool;
    THashMap<TString, TDssmIndexPtr> DssmIndexes;
    size_t Dimension;
    const TVector<float> ZeroEmbedding;
};

using TDssmModelWithIndexesPtr = TIntrusivePtr<TDssmModelWithIndexes>;

}

template<>
inline NNlg::TListOption FromStringImpl<NNlg::TListOption, char>(const char* str, size_t len) {
    if (len == 0) {
        return {};
    }
    NNlg::TListOption result;
    for (auto&& split : StringSplitter(str, len).Split(',')) {
        result.push_back(TString{split.Token()});
    }
    return result;
}
