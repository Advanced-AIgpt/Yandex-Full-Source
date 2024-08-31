#include "search.h"

#include "async_logger.h"
#include "index_data.h"
#include "dssm_model_with_indexes.h"
#include "relevance.h"
#include "relevance_error_request.h"

#include <alice/boltalka/extsearch/base/factors/factor_names.h>
#include <kernel/externalrelev/relev.h>
#include <kernel/searchlog/errorlog.h>
#include <search/reqparam/reqparam.h>
#include <search/request/consumers/treat_params/common/treat_params.h>


namespace NNlg {

namespace {

const TString UNISTAT_PREFIX = "general_conversation_";

struct TInitSearchOptions {
    size_t LoggingQueueSize = 1000;
};

template<class T> TMaybe<T> ParseExperimentValue(const TRequestParams* rp, const TString& prefix) {
    auto expIterator = rp->Experiments.lower_bound(prefix);
    if (expIterator != rp->Experiments.end() && expIterator->StartsWith(prefix)) {
        try {
            return FromString<T>((*expIterator).substr(prefix.size() + 1));
        } catch (const yexception& ex) {}
    }
    return Nothing();
}

void InitSearchOptions(TSearchOptions& options, const TRequestParams* rp) {
#define FILL_OPTION(option) { if (rp->RelevParams.TryGet(#option, value) && value) options.option = FromString<decltype(options.option)>(value); }
    {
        TStringBuf value;
        FILL_OPTION(MaxResults);
        FILL_OPTION(MinRatioWithBestResponse);
        FILL_OPTION(KnnNumCandidates);
        FILL_OPTION(RankerModelName);
        FILL_OPTION(SearchBy);
        FILL_OPTION(SearchFor);
        FILL_OPTION(ContextWeight);
        FILL_OPTION(BoostFactorId);
        FILL_OPTION(BoostFactorWeight);
        FILL_OPTION(UniqueReplies);
        FILL_OPTION(TfRanker);
        FILL_OPTION(TfRankerEnabled);
        FILL_OPTION(TfRankerAlpha);
        FILL_OPTION(TfRankerBoostTop);
        FILL_OPTION(TfRankerBoostRelev);
        FILL_OPTION(TfRankerLogitRelev);
        FILL_OPTION(ProactivityBoost);
        FILL_OPTION(Seq2SeqExternalUri);
        FILL_OPTION(Seq2SeqTimeout);
        FILL_OPTION(UseSeq2SeqDssmEmbedding);
        FILL_OPTION(KnnIndexNames);
        FILL_OPTION(DssmModelNames);
        FILL_OPTION(BaseKnnIndexName);
        FILL_OPTION(BaseDssmModelName);

        FILL_OPTION(BertFactorExternalUri);
        FILL_OPTION(BertFactorEnabled);
        FILL_OPTION(BertFactorRankerModelName);
        FILL_OPTION(BertFactorTimeout);
        FILL_OPTION(BertFactorTopSize);

        FILL_OPTION(RankByLinearCombination);
        FILL_OPTION(LinearCombinationBertCoeff);
        FILL_OPTION(LinearCombinationInformativityCoeff)
        FILL_OPTION(LinearCombinationSeq2SeqCoeff);
        FILL_OPTION(LinearCombinationInterestCoeff);

        FILL_OPTION(BertIsMultitargetHead);
        FILL_OPTION(BertOutputRelevIdx);
        FILL_OPTION(BertOutputInterestIdx);
        FILL_OPTION(BertOutputHasInterest);

        FILL_OPTION(ProactivityIndexEnabled);
        FILL_OPTION(ProactivityPrioritizedInIndex);
        FILL_OPTION(ProactivityBoost);
        FILL_OPTION(Entities);
        FILL_OPTION(EntityBoost);
        FILL_OPTION(Seq2SeqBoost);
        FILL_OPTION(EntityRankerModelName);
        FILL_OPTION(EntityIndexNumCandidates);

        FILL_OPTION(ProtocolRequest);
        FILL_OPTION(RandomSeed);
        FILL_OPTION(UseBaseModelsOnly);
        FILL_OPTION(IndexCandidatesEnabled);
        FILL_OPTION(Seq2SeqCandidatesEnabled);
        FILL_OPTION(DebugStub);
        FILL_OPTION(Seq2SeqPriority);
        FILL_OPTION(Seq2SeqNumHypos);
        //options.MaxResults = rp.GroupingParams[0].gGroups;
    }
#undef FILL_OPTION
}

} // namespace

IRelevance* TNlgSearch::CreateRelevance(const TBaseIndexData& baseIndexData, TIndexAccessors& /*accessors*/, const TRequestParams* rp) const {
    return CreateRelevance(*baseIndexData.GetArchiveManager(), rp);
}

IRelevance* TNlgSearch::CreateRelevance(const TArchiveManager& archiveManager, const TRequestParams* rp) const {
    if (rp->GetRelevParam("ProtocolRequest", false)) {
        return CreateRelevanceProtocol(archiveManager, rp);
    }

    return CreateRelevanceDeprecated(archiveManager, rp);
}

TNlgRelevance* TNlgSearch::CreateRelevanceDeprecated(const TArchiveManager& archiveManager, const TRequestParams* rp) const {
    TSearchOptions options = IndexData.Options;
    InitSearchOptions(options, rp);

    options.RandomSalt = ParseExperimentValue<TString>(rp, "gc_random_salt");

    if (rp->Experiments.count("gc_no_reinforcement")) {
        options.TfRanker = "";
    }

    {
        TMaybe<float> tfRankerAlpha = ParseExperimentValue<float>(rp, "gc_tf_ranker_alpha");
        if (tfRankerAlpha.Defined()) {
            options.TfRankerAlpha = *tfRankerAlpha;
        }
    }

    if (rp->Experiments.count("gc_bigger_top")) {
        options.MaxResults = 20;
    }

    if (rp->Experiments.count("gc_no_seq2seq_external")) {
        options.Seq2SeqCandidatesEnabled = false;
    }

    if (rp->Experiments.count("gc_no_index_candidates")) {
        options.IndexCandidatesEnabled = false;
    }

    if (rp->Experiments.count("gc_bert_factor")) {
        options.BertFactorEnabled = true;
    }

    const auto knnIndexNamesLoaded = options.KnnIndexesToLoad;
    EraseIf(options.KnnIndexNames, [&knnIndexNamesLoaded] (const auto& name) { return !FindPtr(knnIndexNamesLoaded, name); });

    if (!options.RankerModelName.empty() && rp->Experiments.count("gc_proactivity")) {
        TMaybe<float> proactivityBoost = ParseExperimentValue<float>(rp, "gc_proactivity_boost");
        if (proactivityBoost.Defined()) {
            options.ProactivityBoost = *proactivityBoost;
        }
    } else {
        EraseIf(options.KnnIndexNames, [&options] (const auto& name) { return FindPtr(options.ProactivityKnnIndexNames, name); });
    }

    if (options.KnnIndexNames.empty()) {
        options.KnnIndexNames = { options.BaseKnnIndexName };
    }

    // making score-request more user friendly
    if (options.SearchFor == TDssmIndex::ESearchFor::Score) {
        options.SearchBy = TDssmIndex::ESearchBy::ContextAndReply;
    }

    TMnSseDynamicPtr rankerModel;
    if (const auto* rankerModelPtr = IndexData.RankerModels.FindPtr(options.RankerModelName)) {
        rankerModel = *rankerModelPtr;
    }

    TMnSseDynamicPtr bertFactorRankerModel;
    if (const auto* rankerModelPtr = IndexData.RankerModels.FindPtr(options.BertFactorRankerModelName)) {
        bertFactorRankerModel = *rankerModelPtr;
    }

    if (!rankerModel) {
        options.KnnIndexNames = { options.BaseKnnIndexName };
        options.DssmModelNames = { options.BaseDssmModelName };
        options.Seq2SeqCandidatesEnabled = false;
    }

    size_t numStaticFactors = 0;
    if (const auto* numStaticFactorsPtr = IndexData.RankerModelsNumsStaticFactors.FindPtr(options.RankerModelName)) {
        numStaticFactors = *numStaticFactorsPtr;
    }

    TTfRankerPtr tfRanker;
    if (options.TfRankerEnabled) {
        if (const auto* tfRankerPtr = IndexData.TfRankers.FindPtr(options.TfRanker)) {
            tfRanker = *tfRankerPtr;
        }
    }

    if (options.Seq2SeqExternalUri.empty()) {
        options.Seq2SeqCandidatesEnabled = false;
    }

    ELanguage language = rp->QueryLangMask.Count() > 0 ? *rp->QueryLangMask.begin() : options.QueryLanguage;

    return CreateNlgRelevance({
        *rp, options, language, IndexData.DssmModelsWithIndexes, rankerModel, bertFactorRankerModel, numStaticFactors, IndexData.FactorsDssmModels,
        IndexData.FactorCalcer, IndexData.StaticFactors, tfRanker, archiveManager, Seq2SeqExecutor.Get(), Logger.Get(), UnistatRegistry.Get()
    });
}

IRelevance* TNlgSearch::CreateRelevanceProtocol(const TArchiveManager& archiveManager, const TRequestParams* rp) const {
    TSearchOptions options = IndexData.Options;
    const auto knnIndexNamesLoaded = options.KnnIndexesToLoad;
    const auto dssmModelNamesLoaded = options.DssmModelNames;
    InitSearchOptions(options, rp);

    EraseIf(options.KnnIndexNames, [&knnIndexNamesLoaded] (const auto& name) { return !FindPtr(knnIndexNamesLoaded, name); });
    EraseIf(options.DssmModelNames, [&dssmModelNamesLoaded] (const auto& name) { return !FindPtr(dssmModelNamesLoaded, name); });
    if (!options.ProactivityIndexEnabled) {
        EraseIf(options.KnnIndexNames, [&options] (const auto& name) { return FindPtr(options.ProactivityKnnIndexNames, name); });
    }

    if (options.UseBaseModelsOnly) {
        options.KnnIndexNames = { options.BaseKnnIndexName };
        options.DssmModelNames = { options.BaseDssmModelName };
    }

    TMnSseDynamicPtr rankerModel;
    if (options.RankerModelName) {
        if (options.EntityRankerModelName && !options.Entities.empty()) {
            options.RankerModelName = options.EntityRankerModelName;
        }
        const auto* rankerModelPtr = IndexData.RankerModels.FindPtr(options.RankerModelName);
        if (!rankerModelPtr) {
            return CreateNlgErrorHandlerRelevance("Not found RankerModelName: " + options.RankerModelName, Logger.Get(), UnistatRegistry.Get());
        }
        rankerModel = *rankerModelPtr;
    }

    TMnSseDynamicPtr bertFactorRankerModel;
    if (const auto* rankerModelPtr = IndexData.RankerModels.FindPtr(options.BertFactorRankerModelName)) {
        bertFactorRankerModel = *rankerModelPtr;
    }

    if (options.BertFactorEnabled && !(bertFactorRankerModel && options.BertFactorExternalUri)) {
        return CreateNlgErrorHandlerRelevance("Not enough data for BertFactor", Logger.Get(), UnistatRegistry.Get());
    }

    if (options.BertFactorEnabled && options.RankByLinearCombination) {
        return CreateNlgErrorHandlerRelevance("BertFactorEnabled and RankByLinearCombination cannot be used simultaneously", Logger.Get(), UnistatRegistry.Get());
    }

    if (options.LinearCombinationInterestCoeff != 0 && !options.BertOutputHasInterest) {
        return CreateNlgErrorHandlerRelevance("Requested BERT interestingness score is not in BERT output", Logger.Get(), UnistatRegistry.Get());
    }

    TTfRankerPtr tfRanker;
    if (options.TfRankerEnabled && options.TfRanker) {
        const auto* tfRankerPtr = IndexData.TfRankers.FindPtr(options.TfRanker);
        if (!tfRankerPtr) {
            return CreateNlgErrorHandlerRelevance("Not found TfRanker: " + options.TfRanker, Logger.Get(), UnistatRegistry.Get());
        }
        tfRanker = *tfRankerPtr;
    }

    if (options.RankByLinearCombination && tfRanker) {
        return CreateNlgErrorHandlerRelevance("BertFactorEnabled and RL ranker cannot be used simultaneously", Logger.Get(), UnistatRegistry.Get());
    }

    if (options.Seq2SeqCandidatesEnabled && options.Seq2SeqExternalUri.empty()) {
        return CreateNlgErrorHandlerRelevance("No source for Seq2SeqCandidates enabled", Logger.Get(), UnistatRegistry.Get());
    }

    size_t numStaticFactors = 0;
    if (const auto* numStaticFactorsPtr = IndexData.RankerModelsNumsStaticFactors.FindPtr(options.RankerModelName)) {
        numStaticFactors = *numStaticFactorsPtr;
    }

    ELanguage language = rp->QueryLangMask.Count() > 0 ? *rp->QueryLangMask.begin() : options.QueryLanguage;

    return CreateNlgRelevance({
        *rp, options, language, IndexData.DssmModelsWithIndexes, rankerModel, bertFactorRankerModel, numStaticFactors, IndexData.FactorsDssmModels,
        IndexData.FactorCalcer, IndexData.StaticFactors, tfRanker, archiveManager, Seq2SeqExecutor.Get(), Logger.Get(), UnistatRegistry.Get()
    });
}

const IFactorsInfo* TNlgSearch::GetFactorsInfo() const {
    return NNlg::GetNlgFactorsInfo();
}

IIndexData* TNlgSearch::GetIndexData() {
    return &IndexData;
}

void TNlgSearch::Init(const TSearchConfig* config) {
    SEARCH_INFO << "Initializing TNlgSearch from config" << Endl;

    TInitSearchOptions options;
#define FILL_OPTION(option) { if (config->UserDirectives.Get(#option)) options.option = FromString<decltype(options.option)>(config->UserDirectives.Get(#option)); }
    FILL_OPTION(LoggingQueueSize);
#undef FILL_OPTION

    if (options.LoggingQueueSize > 0) {
        Logger = MakeHolder<TAsyncLogger>(options.LoggingQueueSize);
    }
    UnistatRegistry.Reset(new TUnistatRegistry(UNISTAT_PREFIX));

    SEARCH_INFO << "Seq2SeqExecutor threads count: " << config->ProtoCollection_.GetRequestThreads() << Endl;
    Seq2SeqExecutor.Reset(new NPar::TLocalExecutor);
    Seq2SeqExecutor->RunAdditionalThreads(config->ProtoCollection_.GetRequestThreads());
}

void TNlgSearch::PrepareRequestParams(TRequestParams* rp) const {
    rp->NoQtree = true;
    NTreatCgi::SetFakeRequestTree(*rp);
}

void TNlgSearch::TuneRequestParams(TRequestParams* rp) const {
    rp->KeepAllDocuments = true;
}

TNlgSearch* CreateNlgSearch() {
    return new TNlgSearch();
}

TNlgSearch* CreateNlgSearch(const TString& indexDir, const TSearchConfig& config) {
    THolder<TNlgSearch> searcher(new TNlgSearch);
    searcher->Init(&config);
    searcher->GetIndexData()->Open(indexDir.data(), false, &config);
    return searcher.Release();
}

}
