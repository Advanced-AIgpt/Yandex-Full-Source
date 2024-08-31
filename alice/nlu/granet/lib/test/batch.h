#pragma once

#include "dataset.h"
#include "dataset_processor.h"
#include <alice/nlu/granet/lib/grammar/grammar.h>
#include <library/cpp/json/writer/json.h>
#include <library/cpp/langs/langs.h>
#include <util/folder/path.h>
#include <util/generic/noncopyable.h>
#include <util/stream/file.h>
#include <util/stream/output.h>

namespace NGranet {
namespace NBatch {

// ~~~~ ETestCaseType ~~~~

enum ETestCaseType {
    TCT_FORM,
    TCT_ENTITY,
    TCT_USER_ENTITY,
};

// ~~~~ EDatasetRole ~~~~

// Dataset role in TTestCaseConfig.
enum EDatasetRole {
    // Optional (typicaly big common) dataset with entities.
    // If 'base' defined, datasets 'positive' and 'negative' must be subset of 'base'
    // and can omit entities mock.
    DR_BASE,

    // Positive samples with valid tags.
    // If 'base' defined, dataset (Base - Positive) treated as negative.
    DR_POSITIVE,

    // Negative samples.
    DR_NEGATIVE,

    // Samples excluded from testing (for known errors of unit-tests).
    // Can omit entities mock.
    DR_IGNORE,

    DR_COUNT
};

// ~~~~ TTestCaseConfig ~~~~

struct TTestCaseConfig {
public:
    TString CaseName;

    ETestCaseType TaskType = TCT_FORM;
    TString TaskName;

    // Parameters of positive groundtruth generation by base dataset (BaseDataset - Negative - IgnoredSamples).
    ui64 PositiveFromBaseCount = 0;
    double PositiveFromBaseRatio = 0;

    // Parameters of negative groundtruth generation by base dataset (BaseDataset - Positive - IgnoredSamples).
    ui64 NegativeFromBaseCount = 0;
    double NegativeFromBaseRatio = 0;

    ui64 CanonizationLimit = 0;

    EEntitySourceTypes EntitySources = EST_TSV;

    TVector<TString> Datasets[DR_COUNT];
    TString ContextStorage;

    ESampleComponentFlags OutputColumnFilter = SCF_KEY_COLUMNS;

    bool NeedSlots = true;
    bool NeedSlotValues = false;
    bool NeedSlotTypes = false;
    bool NeedSlotValueVariants = false;
    bool CompareSlotsByTop = true;
    bool CollectBlockers = false;
    bool IsVerbose = false;
    bool IsDebug = false;
    bool IsDisabled = false;
    bool DisableAutoTest = false;

public:
    void LoadFromJson(const NJson::TJsonValue& json);
    void SubstTemplate(TStringBuf name, TStringBuf value);
};

// ~~~~ TBatchConfig ~~~~

struct TBatchConfig {
public:
    TGranetDomain Domain;
    size_t SampleCacheSize = 20000;
    size_t DatasetCacheSize = 20;
    TVector<TTestCaseConfig> TestCases;

public:
    void LoadFromBatch(const TFsPath& batchDir, const NJson::TJsonValue& testCaseDefaults = {});
    void LoadFromJsonFile(const TFsPath& path, const NJson::TJsonValue& testCaseDefaults = {});
    void LoadFromJson(const NJson::TJsonValue& json, NJson::TJsonValue testCaseDefaults = {});
};

// ~~~~ TBatchProcessor ~~~~

class TBatchProcessor : public TMoveOnly {
public:
    struct TOptions {
        TFsPath BatchDir;
        TFsPath ResultDir;
        TString Filter;
        TString FilterByNamePrefixIgnoreCase;
        TString ExternalResultColumn;
        ESampleComponentFlags OutputColumnFilter = SCF_KEY_COLUMNS;
        bool NeedSlots = false;
        bool IsAutoTest = false;
        TMaybe<size_t> SampleCacheSize;
    };

public:
    TBatchProcessor(const TOptions& options, IOutputStream* log = nullptr);

    const TBatchConfig& GetConfig() const;
    const TGranetDomain& GetDomain() const;

    void Test(const TGrammar::TConstRef& grammar);
    void CheckFixlist(const TGrammar::TConstRef& grammar);
    void CheckUnitTestLimits();
    void Canonize(const TGrammar::TConstRef& grammar, bool isForFailed);
    void CanonizeTagger(const TGrammar::TConstRef& grammar);
    void UpdateEntities(bool needEmbeddings);

    const TVector<TString>& GetResultUtErrors() const;
    TString GetResultUtMessage() const;
    const TBatchMetrics& GetResultMetrics() const;
    void PrintBlockerSuggests(const TGrammar::TConstRef& grammar, IOutputStream* out, const TString& indent = "") const;

private:
    TDatasetProcessorResult TestSingle(
        const TTestCaseConfig& testCase,
        const TTsvSampleDatasetCachedLoader::TRef& datasetLoader,
        const TPreprocessedSampleCreatorWithCache::TRef& sampleCreator,
        const TFsPath& resultDir) const;
    void WriteResults(const TFsPath& prevResultDir, const TFsPath& currResultDir) const;
    void WriteSummary(const TFsPath& path) const;
    void CopyResultToFixedDir(const TFsPath& currResultDir) const;
    TFsPath TryGetCanonizedDatasetPath(const TTestCaseConfig& testCase, EDatasetRole datasetRole) const;
    bool DetectFailedTestCaseForCanonization(
        const TTestCaseConfig& testCase,
        const TTsvSampleDatasetCachedLoader::TRef& datasetLoader,
        const TPreprocessedSampleCreatorWithCache::TRef& sampleCreator,
        const TFsPath& positive,
        const TFsPath& negative) const;
    void UpdateEntities(bool needEmbeddings, const TTestCaseConfig& testCase,
        EDatasetRole role, THashSet<TString>* donePaths);
    bool ShouldProcessTestCase(const TTestCaseConfig& testCase, TStringBuf datasetName = "") const;
    TSampleProcessor::TRef CreateSampleProcessor(const TTestCaseConfig& testCase,
        const TPreprocessedSampleCreatorWithCache::TRef& sampleCreator) const;
    TDatasetProcessorOptions MakeDatasetProcessorOptions(const TTestCaseConfig& testCase) const;
    TVector<TFsPath> GetDatasets(const TTestCaseConfig& testCase, EDatasetRole role, bool isRequired = false) const;
    TFsPath ResolvePath(const TFsPath& path) const;

private:
    TOptions Options;
    TBatchConfig Config;
    TVector<TString> ResultUtErrors;
    TBatchMetrics ResultMetrics;
    THashMap<TString, TIntentMetrics> FormToMetrics;
    THashMap<TString, TParserBlockerCollector> TestNameToBlockers;
    IOutputStream* Log = nullptr;
};

// ~~~~ Helpers ~~~~

TString EasyUnitTest(const TGrammar::TConstRef& grammar, const TBatchProcessor::TOptions& options);
TString CheckFreshRestrictions(const TGrammar::TConstRef& grammar);

} // namespace NBatch
} // namespace NGranet
