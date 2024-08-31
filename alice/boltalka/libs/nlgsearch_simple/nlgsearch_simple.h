#pragma once

#include <alice/boltalka/extsearch/base/search/search.h>
#include <alice/boltalka/extsearch/base/search/relevance.h>

#include <util/generic/ptr.h>
#include <util/generic/string.h>

namespace NNlg {

struct TNlgSearchSimpleParams {
    TString IndexDir;
    TString MemoryMode = "Precharged";
    TString DssmModelNames = "insight_c3_rus_lister";
    TString BaseDssmModelName = "";
    TString FactorDssmModelNames = "";
    TString RankerModelNamesToLoad = "";
    TString RankerModelName = "";
    TString KnnIndexNames = "base";
    TString BaseKnnIndexName = "";
    TString Seq2SeqExternalUri = "";
    size_t MaxResults = 1;
    TString Experiments = "";
    size_t NumThreads = 20;
};

class TNlgSearchSimple : public TThrRefBase {
public:
    struct TSearchResult {
        float Score;
        TString Reply;
    };

    TNlgSearchSimple(const TNlgSearchSimpleParams& params);
    size_t GetDimension(const TString& dssmModelName) const;
    TVector<float> EmbedContext(const TString& dssmModelName, const TVector<TString>& context) const;
    TVector<float> EmbedReply(const TString& dssmModelName, const TString& reply) const;
    TVector<TString> GetReplyTexts(const TVector<TString>& context) const;
    TVector<const float*> GetReplyEmbeddings(const TString& dssmModelName, const TVector<TString>& context) const;
    TVector<TSearchResult> GetSearchResults(const TVector<TString>& context) const;
    void GetCandidates(const TString& dssmModelName, const TVector<TString>& context, TVector<const float *>* replyEmbeddings, TVector<TString>* replies, TVector<float>* contextEmbedding, TVector<float>* scores) const;

private:
    TString MakeQueryFromContext(const TVector<TString>& context) const;

private:
    THashMap<TString, size_t> Dimensions;
    TArchiveManager ArchiveManager;
    TNlgSearchPtr Searcher;
    TNlgRelevancePtr Relevance;
    THashSet<TString> DssmModelNamesPool;
};


using TNlgSearchSimplePtr = TIntrusivePtr<TNlgSearchSimple>;

}
