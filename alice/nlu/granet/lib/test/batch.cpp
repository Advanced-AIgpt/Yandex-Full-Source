#include "batch.h"
#include "batch_result.h"
#include "dataset_mock_updater.h"
#include <alice/nlu/granet/lib/utils/flag_utils.h>
#include <alice/nlu/granet/lib/utils/json_utils.h>
#include <alice/nlu/granet/lib/utils/utils.h>
#include <alice/nlu/granet/lib/utils/trace.h>
#include <library/cpp/json/json_reader.h>
#include <util/generic/array_ref.h>
#include <util/generic/xrange.h>
#include <util/string/builder.h>
#include <util/string/subst.h>
#include <util/stream/file.h>
#include <util/string/join.h>
#include <util/string/split.h>
#include <util/system/tempfile.h>

namespace NGranet {
namespace NBatch {

using namespace NJson;
using namespace std::literals;

static const size_t FIXLIST_POSITIVE_RATIO_LIMIT = 200;

namespace NScheme {

    namespace NBatchConfig {
        static const TStringBuf Comment = "comment";
        static const TStringBuf Type = "type";
        static const TStringBuf Lang = "language";
        static const TStringBuf IsPASkills = "paskills";
        static const TStringBuf IsWizard = "wizard";
        static const TStringBuf IsSnezhana = "snezhana";
        static const TStringBuf SampleCacheSize = "sample_cache_size";
        static const TStringBuf DatasetCacheSize = "dataset_cache_size";
        static const TStringBuf TestSuites = "suites";
        static const TStringBuf Default = "default";
        static const TStringBuf TestCases = "cases";
        static const TStringBuf Allowed[] = {
            Comment,
            Type,
            Lang,
            IsPASkills,
            IsWizard,
            IsSnezhana,
            SampleCacheSize,
            DatasetCacheSize,
            TestSuites,
            Default,
            TestCases,
        };
    }

    namespace NTestSuite {
        static const TStringBuf Default = "default";
        static const TStringBuf TestCases = "cases";
        static const TStringBuf Allowed[] = {
            Default,
            TestCases,
        };
    }

    namespace NTestCase {
        static const TStringBuf Comment = "comment";
        static const TStringBuf Type = "type";
        static const TStringBuf Verbose = "verbose";
        static const TStringBuf Debug = "debug";
        static const TStringBuf Disable = "disable";
        static const TStringBuf DisableAutoTest = "disable_auto_test";
        static const TStringBuf CaseName = "name";
        static const TStringBuf FormName = "form";
        static const TStringBuf EntityName = "entity";
        static const TStringBuf UserEntityName = "user_entity";
        static const TStringBuf PositiveFromBaseCount = "positive_from_base_count";
        static const TStringBuf PositiveFromBaseRatio = "positive_from_base_ratio";
        static const TStringBuf NegativeFromBaseCount = "negative_from_base_count";
        static const TStringBuf NegativeFromBaseRatio = "negative_from_base_ratio";
        static const TStringBuf CanonizationLimit = "canonization_limit";
        static const TStringBuf EnableTsvEntities = "enable_tsv_entities";
        static const TStringBuf EnableOnlineEntities = "enable_online_entities";
        static const TStringBuf EnableEmptyEntities = "enable_empty_entities";
        static const TStringBuf ContextStorage = "context_storage";
        static const TStringBuf PrintSlots = "print_slots";
        static const TStringBuf PrintSlotValues = "print_slot_values";
        static const TStringBuf PrintSlotTypes = "print_slot_types";
        static const TStringBuf PrintSlotValueVariants = "print_slot_value_variants";
        static const TStringBuf ConsiderSlots = "consider_slots";
        static const TStringBuf ConsiderSlotValues = "consider_slot_values";
        static const TStringBuf ConsiderSlotTypes = "consider_slot_types";
        static const TStringBuf ConsiderSlotValueVariants = "consider_slot_value_variants";
        static const TStringBuf CompareSlotsByTop = "compare_slots_by_top";
        static const TStringBuf CollectBlockers = "collect_blockers";
        static const TStringBuf KeepWeight = "keep_weight";
        static const TStringBuf KeepReqid = "keep_reqid";
        static const TStringBuf KeepContext = "keep_context";
        static const TStringBuf KeepWizextra = "keep_wizextra";
        static const TStringBuf KeepMock = "keep_mock";
        static const TStringBuf KeepEmbeddings = "keep_embeddings";
        static const TStringBuf KeepExtra = "keep_extra";

        static const TStringBuf Allowed[] = {
            Comment,
            Type,
            Verbose,
            Debug,
            Disable,
            DisableAutoTest,
            CaseName,
            FormName,
            EntityName,
            UserEntityName,
            PositiveFromBaseCount,
            PositiveFromBaseRatio,
            NegativeFromBaseCount,
            NegativeFromBaseRatio,
            CanonizationLimit,
            EnableTsvEntities,
            EnableOnlineEntities,
            EnableEmptyEntities,
            ContextStorage,
            PrintSlots,
            PrintSlotValues,
            PrintSlotTypes,
            PrintSlotValueVariants,
            ConsiderSlots,
            ConsiderSlotValues,
            ConsiderSlotTypes,
            ConsiderSlotValueVariants,
            CompareSlotsByTop,
            CollectBlockers,
            KeepWeight,
            KeepReqid,
            KeepContext,
            KeepWizextra,
            KeepMock,
            KeepEmbeddings,
            KeepExtra,
        };
    }

    static const TStringBuf DatasetRoles[DR_COUNT] = {
        "base",
        "positive",
        "negative",
        "ignore"
    };
}

// ~~~~ TTestCaseConfig ~~~~

static void LoadOutputColumnFilterFromJson(const TJsonValue& json, ESampleComponentFlags* components) {
    Y_ENSURE(components);
    SetFlags(components, SCF_WEIGHT, json[NScheme::NTestCase::KeepWeight].GetBooleanSafe(components->HasFlags(SCF_WEIGHT)));
    SetFlags(components, SCF_REQID, json[NScheme::NTestCase::KeepReqid].GetBooleanSafe(components->HasFlags(SCF_REQID)));
    SetFlags(components, SCF_CONTEXT, json[NScheme::NTestCase::KeepContext].GetBooleanSafe(components->HasFlags(SCF_CONTEXT)));
    SetFlags(components, SCF_WIZEXTRA, json[NScheme::NTestCase::KeepWizextra].GetBooleanSafe(components->HasFlags(SCF_WIZEXTRA)));
    SetFlags(components, SCF_MOCK, json[NScheme::NTestCase::KeepMock].GetBooleanSafe(components->HasFlags(SCF_MOCK)));
    SetFlags(components, SCF_EMBEDDINGS, json[NScheme::NTestCase::KeepEmbeddings].GetBooleanSafe(components->HasFlags(SCF_EMBEDDINGS)));
    SetFlags(components, SCF_EXTRA, json[NScheme::NTestCase::KeepExtra].GetBooleanSafe(components->HasFlags(SCF_EXTRA)));
}

static void SaveOutputColumnFilterToJson(const ESampleComponentFlags& components, TJsonValue* json) {
    Y_ENSURE(components);
    (*json)[NScheme::NTestCase::KeepWeight] = components.HasFlags(SCF_WEIGHT);
    (*json)[NScheme::NTestCase::KeepReqid] = components.HasFlags(SCF_REQID);
    (*json)[NScheme::NTestCase::KeepContext] = components.HasFlags(SCF_CONTEXT);
    (*json)[NScheme::NTestCase::KeepWizextra] = components.HasFlags(SCF_WIZEXTRA);
    (*json)[NScheme::NTestCase::KeepMock] = components.HasFlags(SCF_MOCK);
    (*json)[NScheme::NTestCase::KeepEmbeddings] = components.HasFlags(SCF_EMBEDDINGS);
    (*json)[NScheme::NTestCase::KeepExtra] = components.HasFlags(SCF_EXTRA);
}

void TTestCaseConfig::LoadFromJson(const TJsonValue& json) {
    NJsonUtils::CheckAllowedKeys(json, NScheme::NTestCase::Allowed, NScheme::DatasetRoles);

    if (json[NScheme::NTestCase::FormName].GetString(&TaskName)) {
        TaskType = TCT_FORM;
    } else if (json[NScheme::NTestCase::EntityName].GetString(&TaskName)) {
        TaskType = TCT_ENTITY;
    } else if (json[NScheme::NTestCase::UserEntityName].GetString(&TaskName)) {
        TaskType = TCT_USER_ENTITY;
    } else {
        Y_ENSURE(false, "Form or entity name must be defined in test case config");
    }

    CaseName = json[NScheme::NTestCase::CaseName].GetStringSafe(TaskName);

    SetFlags(&EntitySources, EST_TSV,
        json[NScheme::NTestCase::EnableTsvEntities].GetBooleanSafe(EntitySources.HasFlags(EST_TSV)));
    SetFlags(&EntitySources, EST_ONLINE,
        json[NScheme::NTestCase::EnableOnlineEntities].GetBooleanSafe(EntitySources.HasFlags(EST_ONLINE)));
    SetFlags(&EntitySources, EST_EMPTY,
        json[NScheme::NTestCase::EnableEmptyEntities].GetBooleanSafe(EntitySources.HasFlags(EST_EMPTY)));

    PositiveFromBaseCount = json[NScheme::NTestCase::PositiveFromBaseCount].GetUIntegerSafe(0);
    PositiveFromBaseRatio = json[NScheme::NTestCase::PositiveFromBaseRatio].GetDoubleSafe(0);
    NegativeFromBaseCount = json[NScheme::NTestCase::NegativeFromBaseCount].GetUIntegerSafe(0);
    NegativeFromBaseRatio = json[NScheme::NTestCase::NegativeFromBaseRatio].GetDoubleSafe(0);
    Y_ENSURE(PositiveFromBaseRatio >= 0 && PositiveFromBaseRatio <= 1.,
        "Invalid positive_from_base_ratio: " << PositiveFromBaseRatio);
    Y_ENSURE(NegativeFromBaseRatio >= 0 && NegativeFromBaseRatio <= 1.,
        "Invalid negative_from_base_ratio: " << NegativeFromBaseRatio);
    Y_ENSURE((PositiveFromBaseCount == 0 && PositiveFromBaseRatio == 0) ||
        (NegativeFromBaseCount == 0 && NegativeFromBaseRatio == 0),
        "Flags negative_from_base_* conflict with flags positive_from_base_*");

    CanonizationLimit = json[NScheme::NTestCase::CanonizationLimit].GetUIntegerSafe(0);

    for (size_t i = 0; i < DR_COUNT; i++) {
        Datasets[i] = NJsonUtils::GetStrings(json, NScheme::DatasetRoles[i]);
    }

    DisableAutoTest = json[NScheme::NTestCase::DisableAutoTest].GetBooleanSafe(DisableAutoTest);
    ContextStorage = json[NScheme::NTestCase::ContextStorage].GetStringSafe("");
    LoadOutputColumnFilterFromJson(json, &OutputColumnFilter);

    NeedSlots = json[NScheme::NTestCase::PrintSlots].GetBooleanSafe(NeedSlots);
    NeedSlotValues = json[NScheme::NTestCase::PrintSlotValues].GetBooleanSafe(NeedSlotValues);
    NeedSlotTypes = json[NScheme::NTestCase::PrintSlotTypes].GetBooleanSafe(NeedSlotTypes);
    NeedSlotValueVariants = json[NScheme::NTestCase::PrintSlotValueVariants].GetBooleanSafe(NeedSlotValueVariants);

    NeedSlots = json[NScheme::NTestCase::ConsiderSlots].GetBooleanSafe(NeedSlots);
    NeedSlotValues = json[NScheme::NTestCase::ConsiderSlotValues].GetBooleanSafe(NeedSlotValues);
    NeedSlotTypes = json[NScheme::NTestCase::ConsiderSlotTypes].GetBooleanSafe(NeedSlotTypes);
    NeedSlotValueVariants = json[NScheme::NTestCase::ConsiderSlotValueVariants].GetBooleanSafe(NeedSlotValueVariants);

    CompareSlotsByTop = json[NScheme::NTestCase::CompareSlotsByTop].GetBooleanSafe(CompareSlotsByTop);
    CollectBlockers = json[NScheme::NTestCase::CollectBlockers].GetBooleanSafe(CollectBlockers);
    IsVerbose = json[NScheme::NTestCase::Verbose].GetBooleanSafe(IsVerbose);
    IsDebug = json[NScheme::NTestCase::Debug].GetBooleanSafe(IsDebug);
    IsDisabled = json[NScheme::NTestCase::Disable].GetBooleanSafe(IsDisabled);

    IsVerbose = IsVerbose ||
        json[NScheme::NTestCase::PrintSlots].GetBooleanSafe(false) ||
        json[NScheme::NTestCase::PrintSlotValues].GetBooleanSafe(false) ||
        json[NScheme::NTestCase::PrintSlotTypes].GetBooleanSafe(false) ||
        json[NScheme::NTestCase::PrintSlotValueVariants].GetBooleanSafe(false);

    const bool isUt = json[NScheme::NTestCase::Type].GetStringSafe("") == TStringBuf("ut");
    IsVerbose = IsVerbose || isUt;
    IsDebug = IsDebug || isUt;

    SubstTemplate(NScheme::NTestCase::CaseName, CaseName);
    if (TaskType == TCT_FORM) {
        SubstTemplate(NScheme::NTestCase::FormName, TaskName);
        TStringBuf shortName = TaskName;
        shortName.SkipPrefix("personal_assistant."sv);
        SubstTemplate("form_short_name", shortName);
        SubstTemplate("form_without_pa_prefix", shortName);
    } else if (TaskType == TCT_ENTITY) {
        SubstTemplate(NScheme::NTestCase::EntityName, TaskName);
    } else if (TaskType == TCT_USER_ENTITY) {
        SubstTemplate(NScheme::NTestCase::UserEntityName, TaskName);
    }
}

void TTestCaseConfig::SubstTemplate(TStringBuf name, TStringBuf value) {
    const TString what = TString::Join("{", name, "}");
    const TString with(value);
    SubstGlobal(CaseName, what, with);
    SubstGlobal(TaskName, what, with);
    for (TVector<TString>& paths : Datasets) {
        for (TString& path : paths) {
            SubstGlobal(path, what, with);
        }
    }
    SubstGlobal(ContextStorage, what, with);
}

// ~~~~ TBatchConfig ~~~~

void TBatchConfig::LoadFromBatch(const TFsPath& batchDir, const NJson::TJsonValue& testCaseDefaults) {
    LoadFromJsonFile(batchDir / "config.json", testCaseDefaults);
}

void TBatchConfig::LoadFromJsonFile(const TFsPath& path, const NJson::TJsonValue& testCaseDefaults) {
    LoadFromJson(NJsonUtils::ReadJsonFileVerbose(path), testCaseDefaults);
}

void TBatchConfig::LoadFromJson(const TJsonValue& json, NJson::TJsonValue testCaseDefaults) {
    *this = {}; // reset

    NJsonUtils::CheckAllowedKeys(json, NScheme::NBatchConfig::Allowed);
    Domain.Lang = LanguageByName(json[NScheme::NBatchConfig::Lang].GetStringSafe("ru"));
    Y_ENSURE(Domain.Lang != LANG_UNK, "Invalid language name in batch config");
    Domain.IsPASkills = json[NScheme::NBatchConfig::IsPASkills].GetBooleanSafe(false);
    Domain.IsWizard = json[NScheme::NBatchConfig::IsWizard].GetBooleanSafe(false);
    Domain.IsSnezhana = json[NScheme::NBatchConfig::IsSnezhana].GetBooleanSafe(false);

    SampleCacheSize = json[NScheme::NBatchConfig::SampleCacheSize].GetUIntegerSafe(SampleCacheSize);
    DatasetCacheSize = json[NScheme::NBatchConfig::DatasetCacheSize].GetUIntegerSafe(DatasetCacheSize);

    testCaseDefaults[NScheme::NBatchConfig::Type] = json[NScheme::NBatchConfig::Type];
    NJsonUtils::ApplyPatch(json[NScheme::NBatchConfig::Default], &testCaseDefaults);

    for (const TJsonValue& caseJson : json[NScheme::NBatchConfig::TestCases].GetArray()) {
        TJsonValue mergedJson = testCaseDefaults;
        NJsonUtils::ApplyPatch(caseJson, &mergedJson);
        TestCases.emplace_back().LoadFromJson(mergedJson);
    }

    for (const TJsonValue& suiteJson : json[NScheme::NBatchConfig::TestSuites].GetArray()) {
        NJsonUtils::CheckAllowedKeys(suiteJson, NScheme::NTestSuite::Allowed);
        for (const TJsonValue& caseJson : suiteJson[NScheme::NBatchConfig::TestCases].GetArray()) {
            TJsonValue mergedJson = testCaseDefaults;
            NJsonUtils::ApplyPatch(suiteJson[NScheme::NBatchConfig::Default], &mergedJson);
            NJsonUtils::ApplyPatch(caseJson, &mergedJson);
            TestCases.emplace_back().LoadFromJson(mergedJson);
        }
    }
}

// ~~~~ TBatchProcessor ~~~~

TBatchProcessor::TBatchProcessor(const TOptions& options, IOutputStream* log)
    : Options(options)
    , Log(log)
{
    NJson::TJsonValue testCaseDefaults;
    SaveOutputColumnFilterToJson(options.OutputColumnFilter, &testCaseDefaults);
    Config.LoadFromBatch(Options.BatchDir, testCaseDefaults);
    Config.SampleCacheSize = Options.SampleCacheSize.GetOrElse(Config.SampleCacheSize);
}

const TBatchConfig& TBatchProcessor::GetConfig() const {
    return Config;
}

const TGranetDomain& TBatchProcessor::GetDomain() const {
    return Config.Domain;
}

void TBatchProcessor::Test(const TGrammar::TConstRef& grammar) {
    Y_ENSURE(grammar);

    TFsPath prevResultDir;
    TFsPath currResultDir;
    PrepareBatchResultDir(Options.ResultDir, &prevResultDir, &currResultDir);

    TTsvSampleDatasetCachedLoader::TRef datasetLoader = TTsvSampleDatasetCachedLoader::Create(Config.DatasetCacheSize);
    TPreprocessedSampleCreatorWithCache::TRef sampleCreator =
        TPreprocessedSampleCreatorWithCache::Create(grammar, Config.SampleCacheSize);

    for (const TTestCaseConfig& testCase : Config.TestCases) {
        if (!ShouldProcessTestCase(testCase)) {
            continue;
        }
        TRACE_LINE(Log, LogPrefix() << "Test case: " << testCase.CaseName);
        const TDatasetProcessorResult result = TestSingle(testCase, datasetLoader, sampleCreator, currResultDir);
        Extend(result.UnitTestErrors, &ResultUtErrors);
        ResultMetrics.AddIntent(testCase.CaseName, result.Metrics);
        TestNameToBlockers[testCase.CaseName] = result.ParserBlockers;
        if (testCase.TaskType == TCT_FORM) {
            FormToMetrics[testCase.TaskName].AddOther(result.Metrics);
        }
    }
    WriteResults(prevResultDir, currResultDir);
    TRACE_LINE(Log, LogPrefix() << "Done.");
}

TDatasetProcessorResult TBatchProcessor::TestSingle(
    const TTestCaseConfig& testCase,
    const TTsvSampleDatasetCachedLoader::TRef& datasetLoader,
    const TPreprocessedSampleCreatorWithCache::TRef& sampleCreator,
    const TFsPath& resultDir) const
{
    TDatasetProcessorOptions options = MakeDatasetProcessorOptions(testCase);
    options.BaseDatasets = GetDatasets(testCase, DR_BASE);
    options.NegativeTruth = GetDatasets(testCase, DR_NEGATIVE);
    options.PositiveTruth = GetDatasets(testCase, DR_POSITIVE);
    options.IgnoredSamples = GetDatasets(testCase, DR_IGNORE);
    options.PositiveFromBaseCount = testCase.PositiveFromBaseCount;
    options.PositiveFromBaseRatio = testCase.PositiveFromBaseRatio;
    options.NegativeFromBaseCount = testCase.NegativeFromBaseCount;
    options.NegativeFromBaseRatio = testCase.NegativeFromBaseRatio;
    options.IsVerbose = testCase.IsVerbose || Options.IsAutoTest;
    options.IsDebug = testCase.IsDebug;

    if (resultDir.IsDefined()) {
        const TFsPath dir = resultDir / testCase.CaseName;
        options.ResultDatasets[RDT_FALSE_POSITIVE] = dir / "error_excess.tsv";
        options.ResultDatasets[RDT_FALSE_NEGATIVE] = dir / "error_lost.tsv";
        options.ResultDatasets[RDT_RESULT_POSITIVE_DROP_TAGS] = dir / "result_positive.tsv";
        if (options.NeedSlots) {
            options.ResultDatasets[RDT_TAGGER_MISMATCH] = dir / "error_tagger.tsv";
            options.ResultDatasets[RDT_RESULT_POSITIVE] = dir / "result_tagger.tsv";
        }
        options.ReportDir = dir;
    }

    TSampleProcessor::TRef sampleProcessor = CreateSampleProcessor(testCase, sampleCreator);
    const TDatasetProcessorResult result = TDatasetProcessor(options, sampleProcessor, datasetLoader, Log).Process();
    if (result.Metrics.HasAnyError()) {
        const TErrorMetrics& errors = result.Metrics.ErrorsByCount;
        TRACE_LINE(Log, "  Has errors: " <<
            "excess: " << errors.FalsePositive << ", " <<
            "lost: " << errors.FalseNegative << ", " <<
            "tagger: " << errors.TaggerMismatch << ".");
    }
    return result;
}

void TBatchProcessor::WriteResults(const TFsPath& prevResultDir, const TFsPath& currResultDir) const {
    if (!currResultDir.IsDefined()) {
        return;
    }
    TRACE_LINE(Log, LogPrefix() << "Writing results to " << currResultDir);
    WriteSummary(currResultDir / "summary.txt");
    WriteBatchResultDiff(prevResultDir, currResultDir);
    CopyResultToFixedDir(currResultDir);
}

void TBatchProcessor::WriteSummary(const TFsPath& path) const {
    TFileOutput out(path);
    PrintTestingReport(ResultMetrics, &out);
    out << GetResultUtMessage();
}

void TBatchProcessor::CopyResultToFixedDir(const TFsPath& currResultDir) const {
    if (!currResultDir.IsDefined()) {
        return;
    }
    try {
        const TFsPath fixedDir = Options.ResultDir / "last";
        fixedDir.ForceDelete();
        currResultDir.CopyTo(fixedDir, true);
    } catch (...) {
        TRACE_LINE(Log, LogPrefix() << "Failed to rewrite last results: " << CurrentExceptionMessage());
    }
}

void TBatchProcessor::CheckFixlist(const TGrammar::TConstRef& grammar) {
    THashSet<TString> fixlistForms;
    for (const TParserTask& form : grammar->GetData().Forms) {
        if (form.IsFixlist) {
            fixlistForms.insert(form.Name);
        }
    }

    THashSet<TString> testedFixlistForms;
    TTsvSampleDatasetCachedLoader::TRef datasetLoader = TTsvSampleDatasetCachedLoader::Create(Config.DatasetCacheSize);

    for (const TTestCaseConfig& testCase : Config.TestCases) {
        if (!ShouldProcessTestCase(testCase)) {
            continue;
        }
        if (testCase.TaskType != TCT_FORM) {
            continue;
        }
        if (!fixlistForms.contains(testCase.TaskName)) {
            continue;
        }
        testedFixlistForms.insert(testCase.TaskName);
        const TVector<TFsPath> baseDatasets = GetDatasets(testCase, DR_BASE);
        const TVector<TFsPath> negativeDatasets = GetDatasets(testCase, DR_NEGATIVE);
        const TVector<TFsPath> positiveDatasets = GetDatasets(testCase, DR_POSITIVE);
        const TVector<TFsPath> ignoredSamplesDatasets = GetDatasets(testCase, DR_IGNORE);
        if (baseDatasets.size() != 1
            || negativeDatasets.size() != 0
            || positiveDatasets.size() != 1
            || ignoredSamplesDatasets.size() != 0)
        {
            ResultUtErrors.push_back("Bad medium test for fixlist form " + Cite(testCase.TaskName) + "\n");
            continue;
        }
        const double baseWeight = datasetLoader->LoadDataset(baseDatasets[0])->CalculateWeightTotal();
        const double positiveWeight = datasetLoader->LoadDataset(positiveDatasets[0])->CalculateWeightTotal();
        if (positiveWeight * FIXLIST_POSITIVE_RATIO_LIMIT > baseWeight) {
            TStringBuilder out;
            out << "Too many positive matches of fixlist form " << Cite(testCase.TaskName);
            out << ": " << positiveWeight << " of " << baseWeight << " (by weight)\n";
            ResultUtErrors.push_back(out);
        }
    }

    for (const TString& formName : fixlistForms) {
        if (!testedFixlistForms.contains(formName)) {
            ResultUtErrors.push_back("No medium test for fixlist form " + Cite(formName) + "\n");
        }
    }
}

void TBatchProcessor::CheckUnitTestLimits() {
    for (const TTestCaseConfig& testCase : Config.TestCases) {
        if (!ShouldProcessTestCase(testCase)) {
            continue;
        }
        if (testCase.CanonizationLimit == 0) {
            continue;
        }
        for (const TFsPath& path : GetDatasets(testCase, DR_POSITIVE)) {
            TTsv tsv;
            tsv.Read(path);
            if (tsv.GetLines().size() <= testCase.CanonizationLimit) {
                continue;
            }
            TStringBuilder out;
            out << "Error! Too many samples in dataset " << Cite(path) << Endl;
            out << "  Number of samples: " << tsv.GetLines().size() << Endl;
            out << "  Limit for unit tests: " << testCase.CanonizationLimit << Endl;
            out << "  Recanonize your dataset using smaller base dataset" << Endl;
            ResultUtErrors.push_back(out);
        }
    }
}

void TBatchProcessor::Canonize(const TGrammar::TConstRef& grammar, bool isForFailed) {
    Y_ENSURE(grammar);

    TTsvSampleDatasetCachedLoader::TRef datasetLoader = TTsvSampleDatasetCachedLoader::Create(Config.DatasetCacheSize);
    TPreprocessedSampleCreatorWithCache::TRef sampleCreator =
        TPreprocessedSampleCreatorWithCache::Create(grammar, Config.SampleCacheSize);

    for (const TTestCaseConfig& testCase : Config.TestCases) {
        const TFsPath positive = TryGetCanonizedDatasetPath(testCase, DR_POSITIVE);
        const TFsPath negative = TryGetCanonizedDatasetPath(testCase, DR_NEGATIVE);
        if (!positive.IsDefined() && !negative.IsDefined()) {
            continue;
        }
        TRACE_LINE(Log, LogPrefix() << "Test case: " << testCase.CaseName);
        if (testCase.Datasets[DR_BASE].empty()) {
            TRACE_LINE(Log, "  No base dataset.");
            continue;
        }
        if (isForFailed) {
            if (!DetectFailedTestCaseForCanonization(testCase, datasetLoader, sampleCreator, positive, negative)) {
                continue;
            }
            TRACE_LINE(Log, "  Canonization:");
        }
        TDatasetProcessorOptions options = MakeDatasetProcessorOptions(testCase);
        options.OutputColumnFilterFromProcessedDataset = options.OutputColumnFilter;
        options.PositiveTruth = GetDatasets(testCase, DR_BASE);
        options.IgnoredSamples = GetDatasets(testCase, DR_IGNORE);
        options.ResultDatasets[RDT_RESULT_POSITIVE] = positive;
        options.ResultDatasets[RDT_RESULT_NEGATIVE] = negative;

        TSampleProcessor::TRef sampleProcessor = CreateSampleProcessor(testCase, sampleCreator);
        const TDatasetProcessorResult result = TDatasetProcessor(options, sampleProcessor, datasetLoader, Log).Process();
        if (positive.IsDefined()) {
            TRACE_LINE(Log, "  " << result.Metrics.ErrorsByCount.ResultPositive() << " positive samples were found.");
        }
        if (negative.IsDefined()) {
            TRACE_LINE(Log, "  " << result.Metrics.ErrorsByCount.ResultNegative() << " negative samples were found.");
        }
    }
}

TFsPath TBatchProcessor::TryGetCanonizedDatasetPath(const TTestCaseConfig& testCase, EDatasetRole datasetRole) const {
    const TVector<TString>& paths = testCase.Datasets[datasetRole];
    if (paths.size() != 1) {
        return {};
    }
    if (!ShouldProcessTestCase(testCase, paths[0])) {
        return {};
    }
    return ResolvePath(paths[0]);
}

bool TBatchProcessor::DetectFailedTestCaseForCanonization(
    const TTestCaseConfig& testCase,
    const TTsvSampleDatasetCachedLoader::TRef& datasetLoader,
    const TPreprocessedSampleCreatorWithCache::TRef& sampleCreator,
    const TFsPath& positive,
    const TFsPath& negative) const
{
    if (positive.IsDefined() && !positive.Exists()) {
        TRACE_LINE(Log, "  Positive dataset is missing.");
        return true;
    }
    if (negative.IsDefined() && !negative.Exists()) {
        TRACE_LINE(Log, "  Negative dataset is missing.");
        return true;
    }
    try {
        const TDatasetProcessorResult result = TestSingle(testCase, datasetLoader, sampleCreator, {});
        if (result.Metrics.HasAnyError()) {
            return true;
        }
    } catch (...) {
        TRACE_LINE(Log, "  " << CurrentExceptionMessage());
        return true;
    }
    return false;
}

void TBatchProcessor::CanonizeTagger(const TGrammar::TConstRef& grammar) {
    Y_ENSURE(grammar);

    TTsvSampleDatasetCachedLoader::TRef datasetLoader = TTsvSampleDatasetCachedLoader::Create(Config.DatasetCacheSize);
    TPreprocessedSampleCreatorWithCache::TRef sampleCreator =
        TPreprocessedSampleCreatorWithCache::Create(grammar, Config.SampleCacheSize);

    for (const TTestCaseConfig& testCase : Config.TestCases) {
        for (const TString& relativeOriginal : testCase.Datasets[DR_POSITIVE]) {
            if (!ShouldProcessTestCase(testCase, relativeOriginal)) {
                continue;
            }
            const TFsPath original = ResolvePath(relativeOriginal);
            TRACE_LINE(Log, LogPrefix() << "Test case: " << testCase.CaseName);

            const TFsPath tempFile = MakeTempName(nullptr, "granet");
            const TTempFile tempFileRemover(tempFile);

            TDatasetProcessorOptions options = MakeDatasetProcessorOptions(testCase);
            options.OutputColumnFilterFromProcessedDataset = SCF_ALL_COLUMNS;
            options.BaseDatasets = GetDatasets(testCase, DR_BASE);
            options.PositiveTruth.push_back(original);
            options.ResultDatasets[RDT_TAGGER_RESULT] = tempFile;

            TSampleProcessor::TRef sampleProcessor = CreateSampleProcessor(testCase, sampleCreator);
            TDatasetProcessor(options, sampleProcessor, datasetLoader, Log).Process();

            tempFile.ForceRenameTo(original);
        }
    }
}

void TBatchProcessor::UpdateEntities(bool needEmbeddings) {
    THashSet<TString> donePaths;
    for (const TTestCaseConfig& testCase : Config.TestCases) {
        if (!testCase.Datasets[NBatch::DR_BASE].empty()) {
            UpdateEntities(needEmbeddings, testCase, NBatch::DR_BASE, &donePaths);
        } else {
            UpdateEntities(needEmbeddings, testCase, NBatch::DR_POSITIVE, &donePaths);
            UpdateEntities(needEmbeddings, testCase, NBatch::DR_NEGATIVE, &donePaths);
        }
    }
}

void TBatchProcessor::UpdateEntities(bool needEmbeddings, const TTestCaseConfig& testCase,
    EDatasetRole role, THashSet<TString>* donePaths)
{
    for (const TString& relativePath : testCase.Datasets[role]) {
        if (!ShouldProcessTestCase(testCase, relativePath)) {
            continue;
        }
        const TFsPath path = ResolvePath(relativePath);
        if (!TryInsert(path.GetPath(), donePaths)) {
            continue;
        }
        TDatasetMockUpdater::TOptions fetcherOptions = {
            .SrcPath = path,
            .Domain = Config.Domain,
            .ShouldUpdateEmbeddings = needEmbeddings
        };
        TDatasetMockUpdater(fetcherOptions, Log).Update();
    }
}

bool TBatchProcessor::ShouldProcessTestCase(const TTestCaseConfig& testCase, TStringBuf datasetName) const {
    if (testCase.IsDisabled) {
        return false;
    }
    if (testCase.DisableAutoTest && Options.IsAutoTest) {
        return false;
    }
    if (!Options.FilterByNamePrefixIgnoreCase.empty()
        && !to_lower(testCase.CaseName).StartsWith(to_lower(Options.FilterByNamePrefixIgnoreCase)))
    {
        return false;
    }
    const TString& filter = Options.Filter;
    return filter.empty()
        || testCase.CaseName.Contains(filter)
        || testCase.TaskName.Contains(filter)
        || datasetName.Contains(filter);
}

TSampleProcessor::TRef TBatchProcessor::CreateSampleProcessor(const TTestCaseConfig& testCase,
    const TPreprocessedSampleCreatorWithCache::TRef& sampleCreator) const
{
    if (!Options.ExternalResultColumn.empty()) {
        return MakeIntrusive<TSampleProcessorByDatasetColumn>(
            GetDatasets(testCase, DR_BASE), Options.ExternalResultColumn, testCase.TaskName);
    }

    if (testCase.TaskType == TCT_USER_ENTITY) {
        TSampleProcessorByUserEntity::TOptions options;
        options.EntityName = testCase.TaskName;
        options.Domain = Config.Domain;
        options.ContextStoragePath = ResolvePath(testCase.ContextStorage);
        return MakeIntrusive<TSampleProcessorByUserEntity>(options);
    }

    TSampleProcessorByGrammar::TOptions options;
    if (testCase.TaskType == TCT_FORM) {
        options.TaskType = PTT_FORM;
    } else if (testCase.TaskType == TCT_ENTITY) {
        options.TaskType = PTT_ENTITY;
    } else {
        Y_ENSURE(false);
    }
    options.TaskName = testCase.TaskName;
    options.Domain = Config.Domain;
    options.EntitySources = testCase.EntitySources;
    options.ContextStoragePath = ResolvePath(testCase.ContextStorage);
    options.CollectBlockers = testCase.CollectBlockers;
    return MakeIntrusive<TSampleProcessorByGrammar>(options, sampleCreator);
}

TDatasetProcessorOptions TBatchProcessor::MakeDatasetProcessorOptions(const TTestCaseConfig& testCase) const {
    TDatasetProcessorOptions options;
    options.OutputColumnFilter = testCase.OutputColumnFilter;
    options.NeedSlots = testCase.NeedSlots || Options.NeedSlots;
    SetFlags(&options.SlotPrintingOptions, SPO_NEED_VALUES, testCase.NeedSlotValues || testCase.NeedSlotValueVariants);
    SetFlags(&options.SlotPrintingOptions, SPO_NEED_TYPES, testCase.NeedSlotTypes);
    SetFlags(&options.SlotPrintingOptions, SPO_NEED_VARIANTS, testCase.NeedSlotValueVariants);
    options.CompareSlotsByTop = testCase.CompareSlotsByTop;
    options.CollectBlockers = testCase.CollectBlockers;
    return options;
}

TVector<TFsPath> TBatchProcessor::GetDatasets(const TTestCaseConfig& testCase, EDatasetRole role, bool isRequired) const {
    TVector<TFsPath> result;
    for (const TString& path : testCase.Datasets[role]) {
        result.push_back(ResolvePath(path));
    }
    Y_ENSURE(!isRequired || !result.empty(), "Dataset " + Cite(NScheme::DatasetRoles[role]) + " not defined");
    return result;
}

TFsPath TBatchProcessor::ResolvePath(const TFsPath& path) const {
    TFsPath result = (!path.IsDefined() || path.IsAbsolute()) ? path : Options.BatchDir / path;
    return result.Fix();
}

const TVector<TString>& TBatchProcessor::GetResultUtErrors() const {
    return ResultUtErrors;
}

TString TBatchProcessor::GetResultUtMessage() const {
    if (ResultUtErrors.empty()) {
        return "";
    }
    TStringBuilder message;
    message << "Errors in grammar!" << Endl;
    message << JoinSeq("", ResultUtErrors);
    message << "Total error count: " << ResultUtErrors.size() << Endl;
    return message;
}

const TBatchMetrics& TBatchProcessor::GetResultMetrics() const {
    return ResultMetrics;
}

void TBatchProcessor::PrintBlockerSuggests(const TGrammar::TConstRef& grammar, IOutputStream* out, const TString& indent) const {
    Y_ENSURE(out);
    for (const auto& [testName, blockers] : TestNameToBlockers) {
        if (!blockers.HasBlockers()) {
            continue;
        }
        *out << indent << "Suggests for " << testName << ":" << Endl;
        blockers.PrintSuggests(grammar, out, indent);
    }
}

// ~~~~ Helpers ~~~~

TString EasyUnitTest(const TGrammar::TConstRef& grammar, const TBatchProcessor::TOptions& options) {
    TBatchProcessor::TOptions patchedOptions = options;
    patchedOptions.IsAutoTest = true;
    TBatchProcessor tester(patchedOptions);
    tester.Test(grammar);
    return tester.GetResultUtMessage();
}

TString CheckFreshRestrictions(const TGrammar::TConstRef& grammar) {
    if (!grammar->GetDomain().IsAlice()) {
        return "";
    }
    TStringBuilder out;
    for (const TParserTask& form : grammar->GetData().Forms) {
        if (form.Freshness == 0 && !form.Fresh) {
            continue;
        }
        if (form.IsAction || form.IsFixlist) {
            continue;
        }
        out << "Error! Form " << Cite(form.Name)
            << " with fast release option does not have flag is_action or is_fixlist.\n";
    }
    for (const TParserTask& entity : grammar->GetData().Entities) {
        if (entity.Freshness == 0 && !entity.Fresh) {
            continue;
        }
        for (const TParserTask& form : grammar->GetData().Forms) {
            if (!IsIn(form.DependsOnEntities, entity.Name)) {
                continue;
            }
            if (form.IsAction || form.IsFixlist) {
                continue;
            }
            out << "Error! Entity " << Cite(entity.Name)
                << " with fast release option is used in form " << Cite(form.Name)
                << " which does not have flag is_action or is_fixlist.\n";
        }
    }
    return out;
}

} // namespace NBatch
} // namespace NGranet
