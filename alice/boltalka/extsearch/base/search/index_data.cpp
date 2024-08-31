#include "index_data.h"

#include <alice/boltalka/extsearch/base/factors/factor_names.h>
#include <alice/boltalka/extsearch/base/calc_factors/nlg_search_factor_calcer.h>

#include <search/config/virthost.h>

#include <kernel/searchlog/errorlog.h>

#include <util/string/split.h>
#include <util/string/cast.h>
#include <util/folder/path.h>
#include <util/system/mlock.h>

#include <contrib/libs/intel/mkl/include/mkl.h>

template<>
inline ELanguage FromString<ELanguage>(const char* s) {
    if (strlen(s) == 0)
        return ELanguage::LANG_UNK;

    return LanguageByNameOrDie(s);
}

namespace NNlg {

namespace {

THashMap<TString, TDssmIndex::TSearchOptions> ParseIndexConfig(TStringBuf configLine) {
    THashMap<TString, TDssmIndex::TSearchOptions> indexConfigs;
    for (const auto& knnIndexParams : StringSplitter(configLine).Split(',')) {
        TVector<TString> tokens = StringSplitter(knnIndexParams.Token()).Split(':');
        auto knnIndexName = tokens[0];
        TDssmIndex::TSearchOptions currentIndexConfig;
        for (size_t i = 1; i < tokens.size(); ++i) {
            if (tokens[i].StartsWith("sns")) {
                currentIndexConfig.KnnSearchNeighborhoodSize = FromString<size_t>(tokens[i].substr(strlen("sns")));
            } else if (tokens[i].StartsWith("dcl")) {
                currentIndexConfig.KnnDistanceCalcLimit = FromString<size_t>(tokens[i].substr(strlen("dcl")));
            } else if (tokens[i].StartsWith("nc")) {
                currentIndexConfig.KnnNumCandidates = FromString<size_t>(tokens[i].substr(strlen("nc")));
                currentIndexConfig.MaxResults = currentIndexConfig.KnnNumCandidates;
            }
        }
        indexConfigs.emplace(knnIndexName, currentIndexConfig);
    }

    return indexConfigs;
}

}

struct TInitIndexDataOptions {
    size_t MklNumThreads = 1;
    EMemoryMode MemoryMode = EMemoryMode::Locked;
    bool LockBinary = false;
    TListOption TfRankersToLoad;
};

void TIndexData::Open(const char* indexDir, bool /*isPolite*/, const TSearchConfig* config) {
    SEARCH_INFO << "Openning TIndexData" << Endl;

#define FILL_OPTION(option) { if (config->UserDirectives.Get(#option)) Options.option = FromString<decltype(Options.option)>(config->UserDirectives.Get(#option)); }
    FILL_OPTION(MaxResults);
    FILL_OPTION(MinRatioWithBestResponse);
    FILL_OPTION(KnnNumCandidates);
    FILL_OPTION(BaseKnnIndexName);
    FILL_OPTION(BaseDssmModelName);
    FILL_OPTION(KnnIndexNames);
    FILL_OPTION(DssmModelNames);
    FILL_OPTION(FactorDssmModelNames);
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
    FILL_OPTION(QueryLanguage);
    FILL_OPTION(Seq2SeqExternalUri);
    FILL_OPTION(Seq2SeqTimeout);
    FILL_OPTION(UseSeq2SeqDssmEmbedding);

    FILL_OPTION(BertFactorExternalUri);
    FILL_OPTION(BertFactorEnabled);
    FILL_OPTION(BertFactorRankerModelName);
    FILL_OPTION(BertFactorTimeout);
    FILL_OPTION(BertFactorTopSize);

    FILL_OPTION(RankByLinearCombination);
    FILL_OPTION(LinearCombinationBertCoeff);
    FILL_OPTION(LinearCombinationInformativityCoeff);
    FILL_OPTION(LinearCombinationSeq2SeqCoeff);
    FILL_OPTION(LinearCombinationInterestCoeff);

    FILL_OPTION(BertIsMultitargetHead);
    FILL_OPTION(BertOutputRelevIdx);
    FILL_OPTION(BertOutputInterestIdx);
    FILL_OPTION(BertOutputHasInterest);

    FILL_OPTION(ProactivityBoost);
    FILL_OPTION(ProactivityKnnIndexNames);
    FILL_OPTION(EntityBoost);
    FILL_OPTION(Seq2SeqBoost);
    FILL_OPTION(EntityRankerModelName);
    FILL_OPTION(EntityIndexNumCandidates);
    FILL_OPTION(Seq2SeqPriority);
    FILL_OPTION(Seq2SeqNumHypos);
#undef FILL_OPTION

    TInitIndexDataOptions options;
#define FILL_OPTION(option) { if (config->UserDirectives.Get(#option)) options.option = FromString<decltype(options.option)>(config->UserDirectives.Get(#option)); }
    FILL_OPTION(LockBinary);
    FILL_OPTION(MklNumThreads);
    FILL_OPTION(MemoryMode);
    FILL_OPTION(TfRankersToLoad);
#undef FILL_OPTION

    if (options.LockBinary) {
        LockAllMemory(LockCurrentMemory);
    }
    if (Options.DssmModelNames.size() == 1 && Options.BaseDssmModelName.empty()) {
        Options.BaseDssmModelName = Options.DssmModelNames[0];
    }
    Y_ENSURE(std::find(Options.DssmModelNames.begin(), Options.DssmModelNames.end(), Options.BaseDssmModelName) != Options.DssmModelNames.end(), "You should specify base model that should be used in an absense of ranker.");
    SEARCH_INFO << "Base dssm model is " << Options.BaseDssmModelName << Endl;
    const auto knnIndexConfigs = ParseIndexConfig(config->UserDirectives.Get("KnnIndexesToLoad"));
    for (const auto& [indexName, indexConfig] : knnIndexConfigs) {
        Options.KnnIndexesToLoad.push_back(indexName);
    }
    Options.KnnOptions.insert(knnIndexConfigs.begin(), knnIndexConfigs.end());

    const auto entityKnnIndexConfigs = ParseIndexConfig(config->UserDirectives.Get("EntityIndexNames"));
    for (const auto& [indexName, indexConfig] : entityKnnIndexConfigs) {
        Options.EntityIndexNames.push_back(indexName);
    }
    Options.KnnOptions.insert(entityKnnIndexConfigs.begin(), entityKnnIndexConfigs.end());

    if (Options.KnnIndexesToLoad.size() == 1 && Options.BaseKnnIndexName.empty()) {
        Options.BaseKnnIndexName = Options.KnnIndexesToLoad[0];
    }
    Y_ENSURE(std::find(Options.KnnIndexesToLoad.begin(), Options.KnnIndexesToLoad.end(), Options.BaseKnnIndexName) != Options.KnnIndexesToLoad.end(), "You should specify base knn index that should be used in an absense of ranker.");
    SEARCH_INFO << "Base knn index is " << Options.BaseKnnIndexName << Endl;


    const size_t numSearchThreads = Max<size_t>(1, config->ProtoCollection_.GetRequestThreads());
    SEARCH_INFO << "Processing requests in " << numSearchThreads << " threads" << Endl;

    mkl_set_num_threads(options.MklNumThreads);

    // TODO(krom): implying that all DssmModelNames x KnnIndexesToLoad combinations are possible. It's better for models to be independent.
    for (const auto& modelName : Options.DssmModelNames) {
        DssmModelsWithIndexes[modelName] = new TDssmModelWithIndexes(TFsPath(indexDir) / modelName, Options.KnnIndexesToLoad, Options.EntityIndexNames, numSearchThreads, options.MemoryMode);
        Y_ENSURE(DssmModelsWithIndexes[modelName]->GetDssmIndex(Options.BaseKnnIndexName) != nullptr, "At least base knn index is required.");
        SEARCH_INFO << "Loaded " << modelName << " dssm model index" << Endl;
    }
    Y_ENSURE(DssmModelsWithIndexes.contains(Options.BaseDssmModelName), "At least base dssm index is required.");

    for (const auto& modelName : Options.FactorDssmModelNames) {
        FactorsDssmModels[modelName] = new TFactorDssmModel(TFsPath(indexDir) / modelName, numSearchThreads, options.MemoryMode);
        SEARCH_INFO << "Loaded " << modelName << " factor dssm model" << Endl;
    }

    TVector<TString> modelParams = StringSplitter(config->UserDirectives.Get("RankerModelsToLoad")).Split(',');
    for (const TString& modelParam : modelParams) {
        TVector<TString> tokens = StringSplitter(modelParam).Split(':');
        const TString& modelName = tokens[0];
        TFsPath modelFile = TFsPath(indexDir) / (modelName + ".info");
        Y_ENSURE(modelFile.Exists(), "Failed to load ranker '" + modelName + "' - file not found");
        TFileInput in(modelFile);
        auto *modelPtr = new TMnSseDynamic();
        modelPtr->Load(&in);
        RankerModels[modelName] = modelPtr;
        for (size_t i = 1; i < tokens.size(); ++i) {
            if (tokens[i].StartsWith("sf")) {
                RankerModelsNumsStaticFactors[modelName] = FromString<size_t>(tokens[i].substr(strlen("sf")));
            }
        }
        Y_ENSURE(RankerModelsNumsStaticFactors.count(modelName), "No static factors number provided for model " + modelName);
        SEARCH_INFO << "Loaded " << modelName << " ranker model" << Endl;
    }

    TFsPath rusListerMapPath = TFsPath(indexDir) / "ruslister_map.txt";
    Y_ENSURE(rusListerMapPath.Exists(), "No rus lister");
    FactorCalcer = CreateNlgSearchFactorCalcer(rusListerMapPath, Options.DssmModelNames, Options.FactorDssmModelNames);
    SEARCH_INFO << "Created FactorCalcer" << Endl;

    TFsPath staticFactorsPath = TFsPath(indexDir) / "static_factors.bin";
    Y_ENSURE(staticFactorsPath.Exists(), "No static factors");
    StaticFactors = new TStaticFactorsStorage(staticFactorsPath, GetNlgFactorsInfo(), options.MemoryMode);
    SEARCH_INFO << "Loaded " << staticFactorsPath << Endl;

    for (const TString& tfRanker : options.TfRankersToLoad) {
        TFsPath tfRankerDir = tfRanker;
        Y_ENSURE(tfRankerDir.Exists(), "TF ranker not found: " + tfRankerDir.Basename());
        SEARCH_INFO << "Loaded TF ranker '" << tfRankerDir.Basename() << "'" << Endl;
        TfRankers[tfRankerDir.Basename()] = new TTfRanker(tfRankerDir);
    }
}

void TIndexData::Close()
{
}

void TIndexData::SetIndexDataOptions(TBaseIndexDataOptions& options) const {
    options.DocCount = 0;
    options.HasUserData = false;
}

}
