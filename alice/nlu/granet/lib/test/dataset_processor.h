
#pragma once

#include "dataset.h"
#include "dataset_statistics.h"
#include "metrics.h"
#include "sample_processor.h"

namespace NGranet {

// ~~~~ EResultDatasetType ~~~~

enum EResultDatasetType {
    RDT_FALSE_POSITIVE,
    RDT_FALSE_NEGATIVE,
    RDT_TRUE_POSITIVE,
    RDT_TRUE_NEGATIVE,

    // All result positive samples (TruePositive + FalsePositive)
    RDT_RESULT_POSITIVE_DROP_TAGS, // tags are removed from text
    RDT_RESULT_POSITIVE, // text according to option TDatasetProcessorOptions::NeedSlots

    // All result negative samples (TrueNegative + FalseNegative)
    RDT_RESULT_NEGATIVE,

    // True-positive samples with correct/incorrect tagger result.
    RDT_TAGGER_CORRECT,
    RDT_TAGGER_MISMATCH,

    // All samples (positive and negative) with result of tagger.
    RDT_TAGGER_RESULT,

    RDT_COUNT
};

// ~~~~ TDatasetProcessorOptions ~~~~

struct TDatasetProcessorOptions {
    // Groundtruth.
    // Can omit entities (see BaseDatasets).
    TVector<TFsPath> PositiveTruth;
    TVector<TFsPath> NegativeTruth;

    // Ignored samples.
    TVector<TFsPath> IgnoredSamples;

    // Dataset used as source of samples with entities.
    // If list BaseDatasets not empty, PositiveTruth and NegativeTruth must be subset of union of BaseDatasets.
    TVector<TFsPath> BaseDatasets;

    // Parameters of positive groundtruth generation by base dataset (BaseDataset - Negative - IgnoredSamples).
    ui64 PositiveFromBaseCount = 0;
    double PositiveFromBaseRatio = 0;

    // Parameters of negative groundtruth generation by base dataset (BaseDataset - Positive - IgnoredSamples).
    ui64 NegativeFromBaseCount = 0;
    double NegativeFromBaseRatio = 0;

    // Output options.
    TFsPath ResultDatasets[RDT_COUNT];
    TFsPath ReportDir;

    // Filter for set of columns written to ResultDatasets.
    // Allow these columns if processed or base dataset has them.
    ESampleComponentFlags OutputColumnFilter = SCF_KEY_COLUMNS;
    // Allow these columns only if processed dataset (not base dataset) has them.
    ESampleComponentFlags OutputColumnFilterFromProcessedDataset = SCF_KEY_COLUMNS | SCF_WEIGHT;

    size_t ThreadCount = 16;

    // Tagger result components. Writen into column "text".
    bool NeedSlots = false;
    ESlotPrintingOptions SlotPrintingOptions = 0;

    // Check that expected value is at the top, else check 'is in'
    bool CompareSlotsByTop = true;

    bool CollectBlockers = false;

    bool IsVerbose = false;
    bool IsDebug = false;

    // Be verbose.
    bool Verbose = false;
};

// ~~~~ TDatasetProcessorResult ~~~~

struct TDatasetProcessorResult {
    TIntentMetrics Metrics;
    TVector<TString> UnitTestErrors;
    TSlowestSamplesCollector SlowestSamples;
    TElementDictionaryCollector ElementDictionaries;
    TNegativeNgramsCollector NegativeNgrams;
    TParserBlockerCollector ParserBlockers;
};

// ~~~~ TDatasetProcessor ~~~~

class TDatasetProcessor : public TMoveOnly {
public:
    TDatasetProcessor(const TDatasetProcessorOptions& options,
        const TSampleProcessor::TRef& sampleProcessor,
        const TTsvSampleDatasetCachedLoader::TRef& datasetLoader = nullptr,
        IOutputStream* log = nullptr);

    TDatasetProcessorResult Process();

private:
    struct TDatasetInfo {
        bool IsPositive = false;
        TTsvSampleDataset::TConstRef Dataset;
        size_t SamplingSize = 0;
        size_t SamplingStep = 0;
        bool SkipCompleteSamples = false;
    };

    struct TSampleData {
        TTsvSample Sample;
        TSampleTsvLine OutputTsvLine;
        bool IsPositiveExpected;
        TSampleProcessorResult Result;
    };

private:
    void InitialDump();
    void CollectIgnoredSamples();
    void PrepareDatasets();
    void AddProcessedDatasets(const TVector<TFsPath>& pathes, bool isPositive);
    void AddSamplesFromBase(ui64 samplingCount, double samplingRatio, bool isPositive);
    void PrepareOutput();
    TVector<TString> GetOutputColumns() const;
    static TVector<TString> GetDatasetColumnIntersection(const TVector<TTsvSampleDataset::TConstRef>& datasets,
        const ESampleComponentFlags& filter);
    void CreateResultDatasetWriters(const TVector<TString>& columns);
    void ProcessDatasets();
    bool PrepareSampleData(const TDatasetInfo& dataset, size_t index,
        TSampleData* result, TVector<TTsvSampleKey>* notFoundSamples);
    bool CombineSampleWithBase(const TTsvSampleKey& key, TTsvSample* sample, TSampleTsvLine* outputTsvLine);
    void ProcessSampleResult(const TSampleData& data);
    void OutputSample(const TSampleTsvLine& line, const TSampleProcessingInfo& info);
    void OutputSample(const TSampleTsvLine& line, EResultDatasetType dataset);
    void ProcessUnitTest(const TTsvSample& sample, const TSampleProcessingInfo& info,
        const TParserTaskResult::TConstRef& parserResult);
    void ReportProgress();
    void FinalizeProgress();
    void WriteReport() const;
    void DumpNotFoundSamples(const TDatasetInfo& dataset, const TVector<TTsvSampleKey>& samples);

private:
    TDatasetProcessorOptions Options;
    TSampleProcessor::TRef SampleProcessor;
    TTsvSampleDatasetCachedLoader::TRef DatasetLoader;
    IOutputStream* Log = nullptr;

    TVector<TTsvSampleDataset::TConstRef> BaseDatasets;
    TVector<TDatasetInfo> Datasets;

    THashSet<TTsvSampleKey> IgnoredSamples;

    // Already processed samples.
    THashSet<TTsvSampleKey> CompleteSamples;

    // For reporting progress to log:
    //   Progress: 100...
    size_t ProgressCounter = 0;
    bool IsInReportingProgress = false;

    TSampleTsvHeader::TConstRef OutputTsvHeader;
    TMaybe<TTsvWriter<ESampleColumnId>> Writers[RDT_COUNT];

    TDatasetProcessorResult Result;
};

} // namespace NGranet
