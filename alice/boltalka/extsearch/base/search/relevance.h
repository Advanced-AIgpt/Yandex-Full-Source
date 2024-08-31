#pragma once
#include "async_logger.h"
#include "dssm_model_with_indexes.h"
#include "fixlist.h"
#include "index_data.h"
#include "static_factors.h"
#include "unistat_registry.h"

#include <alice/boltalka/libs/factors/proto/nlgsearch_factors.pb.h>

#include <kernel/externalrelev/relev.h>
#include <search/config/virthost.h>
#include <search/rank/srchdoc.h>

#include <library/cpp/langs/langs.h>
#include <library/cpp/threading/future/future.h>
#include <library/cpp/threading/local_executor/local_executor.h>

#include <util/generic/ptr.h>
#include <util/generic/maybe.h>

class TArchiveManager;

namespace NNlg {

struct TNlgRelevanceCtx {
    const TRequestParams& RP;
    const TSearchOptions& Options;
    const ELanguage Lang;
    const THashMap<TString, TDssmModelWithIndexesPtr>& DssmModelsWithIndexes;
    const TMnSseDynamicPtr& RankerModel;
    const TMnSseDynamicPtr& BertFactorRankerModel;
    const size_t NumStaticFactors;
    const THashMap<TString, TFactorDssmModelPtr>& FactorDssmModels;
    const TFactorCalcerPtr& FactorCalcer;
    const TStaticFactorsStoragePtr& StaticFactors;
    const TTfRankerPtr& TfRanker;
    const TArchiveManager& Archive;
    NPar::TLocalExecutor* Seq2SeqExecutor;
    TAsyncLogger* Logger;
    TUnistatRegistry* UnistatRegistry;
};

struct TSearchResult : TDssmModelWithIndexes::TSearchResult {
    float TfRankerScore;
    TString DssmIndexName;
    TSearchResult() = default;
    TSearchResult(const TDssmModelWithIndexes::TSearchResult& dssmSearchResult, const TString& dssmIndexName)
        : TDssmModelWithIndexes::TSearchResult(dssmSearchResult)
        , DssmIndexName(dssmIndexName)
    {
    }
};

struct TSearchResultDocs {
    TSearchResultDocs() = default;

    TSearchResultDocs(TVector<TSearchResult>& docs, TVector<TVector<TString>>& contexts, TVector<TString>& replies, TVector<TString>& displayReplies, THashMap<TString, TVector<TVector<float>>>& Seq2SeqReplyEmbeddings, TVector<TSeq2SeqFactorCtx> Seq2SeqFactorCtxs)
        : Docs(std::move(docs))
        , Contexts(std::move(contexts))
        , Replies(std::move(replies))
        , DisplayReplies(std::move(displayReplies))
        , Seq2SeqReplyEmbeddings(std::move(Seq2SeqReplyEmbeddings))
        , Seq2SeqFactorCtxs(std::move(Seq2SeqFactorCtxs))
    {
    }

    void Add(TSearchResultDocs& second) {
        Docs.reserve(Docs.size() + second.Docs.size());
        Contexts.reserve(Contexts.size() + second.Contexts.size());
        Replies.reserve(Replies.size() + second.Replies.size());
        DisplayReplies.reserve(DisplayReplies.size() + second.DisplayReplies.size());
        Seq2SeqFactorCtxs.reserve(Seq2SeqFactorCtxs.size() + second.Seq2SeqFactorCtxs.size());

        std::move(second.Docs.begin(), second.Docs.end(), std::back_inserter(Docs));
        std::move(second.Contexts.begin(), second.Contexts.end(), std::back_inserter(Contexts));
        std::move(second.Replies.begin(), second.Replies.end(), std::back_inserter(Replies));
        std::move(second.DisplayReplies.begin(), second.DisplayReplies.end(), std::back_inserter(DisplayReplies));
        std::move(second.Seq2SeqFactorCtxs.begin(), second.Seq2SeqFactorCtxs.end(), std::back_inserter(Seq2SeqFactorCtxs));

        if (Seq2SeqReplyEmbeddings.empty()) {
            Seq2SeqReplyEmbeddings = std::move(second.Seq2SeqReplyEmbeddings);
        } else {
            Y_ASSERT(Seq2SeqReplyEmbeddings.size() == second.Seq2SeqReplyEmbeddings.size());
            for (auto& [dssmName, vec] : second.Seq2SeqReplyEmbeddings) {
                auto& thisVec = Seq2SeqReplyEmbeddings[dssmName];
                thisVec.reserve(thisVec.size() + vec.size());
                std::move(vec.begin(), vec.end(), std::back_inserter(thisVec));
            }
        }
    }

    void Finalize() {
        for (size_t i = 0; i < Docs.size(); ++i) {
            DocIdToIdx.emplace(Docs[i].DocId, i);
            DocIdToFactors.emplace(Docs[i].DocId, NAlice::NBoltalka::TNlgSearchFactors{});
        }
    }

    void SetDocFactors(ui32 docId, NAlice::NBoltalka::TNlgSearchFactors&& value) {
        DocIdToFactors[docId] = std::move(value);
    }

    TVector<TSearchResult> Docs;
    TVector<TVector<TString>> Contexts;
    TVector<TString> Replies;
    TVector<TString> DisplayReplies;
    THashMap<TString, TVector<TVector<float>>> Seq2SeqReplyEmbeddings;
    TVector<TSeq2SeqFactorCtx> Seq2SeqFactorCtxs;

    THashMap<ui32, size_t> DocIdToIdx;
    THashMap<ui32, NAlice::NBoltalka::TNlgSearchFactors> DocIdToFactors;
};

struct TSeq2SeqAsyncResult {
    TSearchResultDocs ResultDocs;
    double TimeElapsed = 0;
    bool HasError = false;
};

using TSeq2SeqFuture = NThreading::TFuture<TSeq2SeqAsyncResult>;
using TBertOutput = TVector<float>;

class TNlgRelevance : public TThrRefBase, public IRelevance {
public:
    TNlgRelevance(const TNlgRelevanceCtx& ctx);

    bool CalcFilteringResults(TCalcFilteringResultsContext& ctx) override;
    const IFactorsInfo* GetFactorsInfo() const override;
    void CalcFactors(TCalcFactorsContext& ctx) override;
    void CalcRelevance(TCalcRelevanceContext& ctx) override;

    TSearchResultDocs CalcFilteringResultsForQuery(const TString& query);
    void GetTextsFromArchive(const TVector<TSearchResult>& docs, TVector<TVector<TString>>* contexts, TVector<TString>* replies, TVector<TString>* displayReplies) const;
    void GetIsDssmIndexFactor(const TVector<ui32>& dssmIds, TFactorCalcerCtx* ctx) const;

    TVector<TVector<float>> GetIndexQueryEmbeddings(const TString& dssmIndexName, const TVector<TString>& context) const;
    TVector<float> GetIndexReplyEmbedding(const TString& dssmIndexName, const TString& reply) const;
    void GetEmbeddingsFromKnnIndex(const TString& dssmIndexName, const TSearchResultDocs& docs, TVector<const float*>* contextEmbeddings, TVector<const float*>* replyEmbeddings, size_t dimension) const;
    size_t GetDimension(const TString& dssmIndexName) const;

private:
    TVector<TString> TransformContext(TVector<TString> context) const;
    THashMap<TString, TVector<TVector<float>>> GetQueryEmbeddings(const TVector<TString>& context) const;
    TSearchResultDocs GetDocsFromIndex(const THashMap<TString, TVector<TVector<float>>>& queryEmbeddings) const;
    TSearchResultDocs GetDocsFromSeq2Seq(const TVector<TString>& context) const;
    TSearchResultDocs GetFixlistDocs(const TVector<TString>& context) const;

    TSeq2SeqFuture GetDocsFromSeq2SeqAsync(const TVector<TString>& context) const;

    TVector<TBertOutput> GetBertScoresByThreshold(const TVector<TString>& queryContext, const TVector<TString>& replies, const TVector<double>& relevs, double relevThreshold) const;
    void RerankWithBertFactor(const TVector<TString>& queryContext, const TVector<TVector<float>>& factors, const TVector<TString>& replies, TVector<double>* relevs) const;
    void RerankByLinearCombination(const TVector<TString>& queryContext, const TVector<TString>& replies, const TVector<TVector<float>>& factors, TVector<double>* relevs) const;
    void RerankDocs(const TVector<TString>& queryContext, const THashMap<TString, TVector<TVector<float>>>& queryEmbeddings, TFactorCalcerCtx* ctx, TSearchResultDocs& docs) const;
    void GetEmbeddingsFromFactorDssms(const TSearchResultDocs& docs, const TVector<TString>& queryContext, TList<TVector<float>>* queryEmbeddings, TFactorCalcerCtx* ctx) const;
    void GetStaticFactors(const TVector<TSearchResult>& docs, TVector<TVector<float>>* factors, TFactorCalcerCtx* ctx) const;
    void GetInformativenessDenominators(const TVector<TSearchResult>& docs, TFactorCalcerCtx* ctx) const;

    float GetFactorValue(const TVector<float>& factors, const TString& factorName, size_t factorShift = 0) const;

private:
    TSearchOptions Options;
    const NNlgTextUtils::TNlgSearchContextTransform ContextTransform;
    const NNlgTextUtils::TNlgSearchUtteranceTransform ReplyTransform;
    const THashMap<TString, TDssmModelWithIndexesPtr>& DssmModelsWithIndexes;
    TMnSseDynamicPtr RankerModel;
    TMnSseDynamicPtr BertFactorRankerModel;
    const size_t NumStaticFactors;
    const THashMap<TString, TFactorDssmModelPtr>& FactorDssmModels;
    TFactorCalcerPtr FactorCalcer;
    TStaticFactorsStoragePtr StaticFactors;
    TTfRankerPtr TfRanker;
    const bool Seq2SeqCandidatesEnabled;
    const bool IndexCandidatesEnabled;
    const TArchiveManager& Archive;
    NPar::TLocalExecutor* Seq2SeqExecutor;
    TAsyncLogger* Logger;
    TUnistatRegistry* UnistatRegistry;
    const TVector<float> ZeroStaticFactors;
    TFixlist Fixlist;
};

TNlgRelevance* CreateNlgRelevance(const TNlgRelevanceCtx& ctx);

using TNlgRelevancePtr = TIntrusivePtr<TNlgRelevance>;

} // namespace NNlg
