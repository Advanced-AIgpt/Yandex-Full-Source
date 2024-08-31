#pragma once
#include "dssm_model_with_indexes.h"
#include "factor_dssm_model.h"
#include "static_factors.h"
#include "tf_ranker.h"

#include <alice/boltalka/extsearch/base/calc_factors/factor_calcer.h>

#include <kernel/externalrelev/relev.h>
#include <kernel/matrixnet/mn_dynamic.h>
#include <search/rank/baseindexdata.h>

#include <library/cpp/langs/langs.h>

#include <util/generic/maybe.h>
#include <util/memory/blob.h>

namespace NNlg {

struct TSearchOptions : TDssmModelWithIndexes::TSearchOptions {
    TString BaseDssmModelName;
    TListOption DssmModelNames;
    TListOption FactorDssmModelNames;
    TListOption KnnIndexesToLoad;
    TString RankerModelName;
    ELanguage QueryLanguage;
    size_t BoostFactorId = Max<size_t>();
    float BoostFactorWeight = 0.0;
    bool UniqueReplies = false;
    TString TfRanker = "";
    bool TfRankerEnabled = false;
    float TfRankerAlpha = 0.0;
    bool TfRankerBoostTop = false;
    bool TfRankerBoostRelev = false;
    bool TfRankerLogitRelev = false;
    TString Seq2SeqExternalUri;
    ui64 Seq2SeqTimeout = 100;
    bool UseSeq2SeqDssmEmbedding = true;
    TMaybe<TString> RandomSalt;
    bool IndexCandidatesEnabled = true;
    bool Seq2SeqCandidatesEnabled = true;

    TString BertFactorExternalUri;
    bool BertFactorEnabled = false;
    TString BertFactorRankerModelName;
    ui64 BertFactorTimeout = 50;
    size_t BertFactorTopSize = 64;

    bool RankByLinearCombination = false;
    double LinearCombinationBertCoeff = 1.0;
    double LinearCombinationInformativityCoeff = 0.0;
    double LinearCombinationSeq2SeqCoeff = 0.0;
    double LinearCombinationInterestCoeff = 0.0;

    bool BertIsMultitargetHead = false;
    size_t BertOutputRelevIdx = 0;
    size_t BertOutputInterestIdx = 2;
    bool BertOutputHasInterest = false;

    bool ProtocolRequest = false;
    ui64 RandomSeed = 0;
    bool UseBaseModelsOnly = false;

    ui64 DebugStub = 0;
    bool ProactivityIndexEnabled = true;
    bool ProactivityPrioritizedInIndex = true;
    float ProactivityBoost = 0.0;
    TListOption ProactivityKnnIndexNames;
    TListOption Entities;
    float EntityBoost = 0.0;
    float Seq2SeqBoost = 0.0;
    TString EntityRankerModelName = "";
    bool Seq2SeqPriority = false;
    int Seq2SeqNumHypos = 1;
};

using NMatrixnet::TMnSseDynamic;
using NMatrixnet::TMnSseDynamicPtr;

class TIndexData : public IIndexData {
public:
    void Open(const char* indexDir, bool isPolite, const TSearchConfig* config) override;
    void Close() override;

    void SetIndexDataOptions(TBaseIndexDataOptions& options) const override;

    TSearchOptions Options;
    THashMap<TString, TDssmModelWithIndexesPtr> DssmModelsWithIndexes;
    THashMap<TString, TMnSseDynamicPtr> RankerModels;
    THashMap<TString, size_t> RankerModelsNumsStaticFactors;
    THashMap<TString, TFactorDssmModelPtr> FactorsDssmModels;
    TFactorCalcerPtr FactorCalcer;
    TStaticFactorsStoragePtr StaticFactors;
    THashMap<TString, TTfRankerPtr> TfRankers;
};

}
