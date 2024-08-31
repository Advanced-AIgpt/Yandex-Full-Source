#include "dataset_processor.h"
#include <alice/nlu/granet/lib/parser/parser.h>
#include <alice/nlu/granet/lib/test/metrics.h>
#include <alice/nlu/granet/lib/utils/parallel_processor.h>
#include <alice/nlu/granet/lib/utils/trace.h>
#include <dict/nerutil/tstimer.h>
#include <library/cpp/iterator/enumerate.h>
#include <library/cpp/iterator/zip.h>
#include <util/generic/algorithm.h>
#include <util/generic/cast.h>
#include <util/generic/xrange.h>
#include <util/generic/ymath.h>
#include <util/string/builder.h>
#include <util/string/printf.h>
#include <util/string/strip.h>

namespace NGranet {

// ~~~~ TDatasetProcessor ~~~~

TDatasetProcessor::TDatasetProcessor(const TDatasetProcessorOptions& options,
        const TSampleProcessor::TRef& sampleProcessor,
        const TTsvSampleDatasetCachedLoader::TRef& datasetLoader,
        IOutputStream* log)
    : Options(options)
    , SampleProcessor(sampleProcessor)
    , DatasetLoader(datasetLoader)
    , Log(log)
{
    Y_ENSURE(SampleProcessor);
    if (DatasetLoader == nullptr) {
        DatasetLoader = TTsvSampleDatasetCachedLoader::Create();
    }
}

TDatasetProcessorResult TDatasetProcessor::Process() {
    DEBUG_TIMER("NGranet::TDatasetProcessor::Process");

    InitialDump();
    CollectIgnoredSamples();
    PrepareDatasets();
    PrepareOutput();
    ProcessDatasets();
    FinalizeProgress();
    WriteReport();

    return std::move(Result);
}

void TDatasetProcessor::InitialDump() {
    if (!Log || !Options.Verbose) {
        return;
    }
    *Log << "  Datasets:" << Endl;
    for (const TFsPath& path : Options.BaseDatasets) {
        *Log << "    Base dataset:   " << path << Endl;
    }
    for (const TFsPath& path : Options.PositiveTruth) {
        *Log << "    Positive truth: " << path << Endl;
    }
    for (const TFsPath& path : Options.NegativeTruth) {
        *Log << "    Negative truth: " << path << Endl;
    }
    for (const TFsPath& path : Options.IgnoredSamples) {
        *Log << "    Ignored samples:  " << path << Endl;
    }
}

void TDatasetProcessor::CollectIgnoredSamples() {
    DEBUG_TIMER("NGranet::TDatasetProcessor::CollectIgnoredSamples");

    for (const TFsPath& path : Options.IgnoredSamples) {
        const TTsvSampleDataset dataset(path);
        for (const size_t i : xrange(dataset.Size())) {
            const TTsvSampleKey key = dataset.ReadSample(i).MakeKey();
            IgnoredSamples.insert(key);
            CompleteSamples.insert(key);
        }
    }
}

void TDatasetProcessor::PrepareDatasets() {
    DEBUG_TIMER("NGranet::TDatasetProcessor::PrepareDatasets");

    for (const TFsPath& path : Options.BaseDatasets) {
        BaseDatasets.push_back(DatasetLoader->LoadDataset(path));
    }
    AddProcessedDatasets(Options.PositiveTruth, true);
    AddProcessedDatasets(Options.NegativeTruth, false);
    AddSamplesFromBase(Options.PositiveFromBaseCount, Options.PositiveFromBaseRatio, true);
    AddSamplesFromBase(Options.NegativeFromBaseCount, Options.NegativeFromBaseRatio, false);
}

void TDatasetProcessor::AddProcessedDatasets(const TVector<TFsPath>& pathes, bool isPositive) {
    DEBUG_TIMER("NGranet::TDatasetProcessor::AddProcessedDatasets");

    for (const TFsPath& path : pathes) {
        TDatasetInfo dataset;
        dataset.IsPositive = isPositive;
        dataset.Dataset = DatasetLoader->LoadDataset(path);
        dataset.SamplingSize = dataset.Dataset->Size();
        dataset.SamplingStep = 1;
        dataset.SkipCompleteSamples = false;
        Datasets.push_back(std::move(dataset));
    }
}

void TDatasetProcessor::AddSamplesFromBase(ui64 samplingCount, double samplingRatio, bool isPositive) {
    DEBUG_TIMER("NGranet::TDatasetProcessor::AddSamplesFromBase");

    if (samplingCount == 0 && samplingRatio == 0) {
        return;
    }
    for (const TTsvSampleDataset::TConstRef& baseDataset : BaseDatasets) {
        TDatasetInfo dataset;
        dataset.IsPositive = isPositive;
        dataset.Dataset = baseDataset;
        const size_t srcSize = dataset.Dataset->Size();
        dataset.SamplingSize = Min(srcSize, Max<size_t>(samplingCount, static_cast<size_t>(round(samplingRatio * srcSize))));
        dataset.SamplingStep = NthBit64(MostSignificantBit(srcSize / Max<size_t>(1, dataset.SamplingSize)));
        Y_ENSURE(dataset.SamplingSize == 0 || dataset.SamplingStep > 0);
        dataset.SkipCompleteSamples = true;
        Datasets.push_back(std::move(dataset));
    }
}

void TDatasetProcessor::PrepareOutput() {
    DEBUG_TIMER("NGranet::TDatasetProcessor::PrepareOutput");

    const TVector<TString> columns = GetOutputColumns();

    OutputTsvHeader = TSampleTsvHeader::Create(columns);
    Y_ENSURE(OutputTsvHeader->HasColumn(ESampleColumnId::Text));

    CreateResultDatasetWriters(columns);
}

TVector<TString> TDatasetProcessor::GetOutputColumns() const {
    Y_ENSURE(!Datasets.empty(), "No datasets");

    TVector<TTsvSampleDataset::TConstRef> processedDatasets;
    for (const TDatasetInfo& dataset : Datasets) {
        processedDatasets.push_back(dataset.Dataset);
    }

    TVector<TString> columns = GetDatasetColumnIntersection(processedDatasets,
        Options.OutputColumnFilterFromProcessedDataset | Options.OutputColumnFilter);

    for (const TString& column : GetDatasetColumnIntersection(BaseDatasets, Options.OutputColumnFilter)) {
        if (IsIn(columns, column)) {
            continue;
        }
        if (column == GetColumnName(ESampleColumnId::Weight)) {
            columns.insert(columns.begin(), column);
        } else {
            columns.push_back(column);
        }
    }

    return columns;
}

// static
TVector<TString> TDatasetProcessor::GetDatasetColumnIntersection(
    const TVector<TTsvSampleDataset::TConstRef>& datasets,
    const ESampleComponentFlags& filter)
{
    TVector<TString> columns;
    bool isFirst = true;
    for (const TTsvSampleDataset::TConstRef& dataset : datasets) {
        if (isFirst) {
            columns = dataset->GetHeader()->GetNames();
            isFirst = false;
        } else {
            EraseIf(columns, [&](const TString& column) {
                return !IsIn(dataset->GetHeader()->GetNames(), column);
            });
        }
    }
    EraseIf(columns, [&](const TString& column) {
        return !IsColumnInComponentSet(filter, column);
    });
    return columns;
}

void TDatasetProcessor::CreateResultDatasetWriters(const TVector<TString>& columns) {
    for (size_t i : xrange(static_cast<size_t>(RDT_COUNT))) {
        const TFsPath& path = Options.ResultDatasets[i];
        if (!path.IsDefined()) {
            continue;
        }
        path.Parent().MkDirs();
        Writers[i].ConstructInPlace(path, columns);
    }
}

void TDatasetProcessor::ProcessDatasets() {
    DEBUG_TIMER("NGranet::TDatasetProcessor::ProcessDatasets");

    for (const TDatasetInfo& dataset : Datasets) {
        TVector<TTsvSampleKey> notFoundSamples;

        THolder<IThreadPool> threadPool = CreateThreadPool(Options.ThreadCount);
        TParallelProcessorWithOrderedPostprocess processor(
            /* process= */ [this](TSampleData data) {
                data.Result = SampleProcessor->ProcessSample(data.Sample, data.IsPositiveExpected, nullptr);
                return data;
            },
            /* postprocess= */ [this](const TSampleData& data) {
                ProcessSampleResult(data);
            },
            /* threadPool= */ threadPool.Get(),
            /* resultQueueLimit= */ 1000
        );

        for (size_t i : xrange(dataset.SamplingSize)) {
            TSampleData sample;
            if (!PrepareSampleData(dataset, i * dataset.SamplingStep, &sample, &notFoundSamples)) {
                continue;
            }
            processor.Push(std::move(sample));
        }
        processor.Finalize();
        DumpNotFoundSamples(dataset, notFoundSamples);
    }
}

bool TDatasetProcessor::PrepareSampleData(const TDatasetInfo& dataset, size_t index,
    TSampleData* result, TVector<TTsvSampleKey>* notFoundSamples)
{
    Y_ENSURE(result);
    Y_ENSURE(notFoundSamples);
    result->Sample = dataset.Dataset->ReadSample(index);

    const TTsvSampleKey key = result->Sample.MakeKey();
    if ((dataset.SkipCompleteSamples ? CompleteSamples : IgnoredSamples).contains(key)) {
        return false;
    }
    CompleteSamples.insert(key);

    result->OutputTsvLine = TSampleTsvLine(OutputTsvHeader, result->Sample.TsvLine);
    if (!CombineSampleWithBase(key, &result->Sample, &result->OutputTsvLine)) {
        notFoundSamples->push_back(key);
    }

    result->IsPositiveExpected = dataset.IsPositive;
    return true;
}

bool TDatasetProcessor::CombineSampleWithBase(const TTsvSampleKey& key, TTsvSample* sample,
    TSampleTsvLine* outputTsvLine)
{
    Y_ENSURE(sample);
    Y_ENSURE(outputTsvLine);

    if (BaseDatasets.empty()) {
        return true;
    }
    bool hasBaseSample = false;
    for (const TTsvSampleDataset::TConstRef& baseDataset : BaseDatasets) {
        const size_t index = baseDataset->FindSample(key);
        if (index == NPOS) {
            continue;
        }
        const TTsvSample baseSample = baseDataset->ReadSample(index);

        for (const auto& [column, value] : Zip(baseSample.TsvLine.GetNames(), baseSample.TsvLine.GetValues())) {
            if (outputTsvLine->GetHeader()->HasColumn(column)
                && !sample->TsvLine.GetHeader()->HasColumn(column))
            {
                (*outputTsvLine)[column] = value;
            }
        }

        if (sample->WeightStr.empty()) {
            sample->WeightStr = baseSample.WeightStr;
            sample->Weight = baseSample.Weight;
        }
        if (sample->Context.empty()) {
            sample->Context = baseSample.Context;
        }
        if (sample->Wizextra.empty()) {
            sample->Wizextra = baseSample.Wizextra;
        }
        if (sample->Mock.empty()) {
            sample->Mock = baseSample.Mock;
        }
        if (sample->Embeddings.empty()) {
            sample->Embeddings = baseSample.Embeddings;
        }
        hasBaseSample = true;
    }
    return hasBaseSample;
}

void TDatasetProcessor::ProcessSampleResult(const TSampleData& data) {
    try {
        TSampleProcessingInfo info = {
            .Weight = data.Sample.Weight,
            .Expected = ReadSampleMarkup(data.IsPositiveExpected, data.Sample.TaggedText),
            .Actual = data.Result.Result,
            .CompareSlotsByTop = Options.CompareSlotsByTop,
            .Time = data.Result.Time
        };
        if (!Options.NeedSlots) {
            info.Actual.Slots.clear();
        }

        OutputSample(data.OutputTsvLine, info);
        ProcessUnitTest(data.Sample, info, data.Result.ParserResult);
        Result.Metrics.AddSample(info);
        if (Options.ReportDir.IsDefined()) {
            Result.SlowestSamples.Accumulate(info.Expected.Text, info.Time);
            Result.ElementDictionaries.Accumulate(info.Actual);
        }
        if (Options.CollectBlockers) {
            Result.ParserBlockers.Accumulate(data.Result);
        }
        ReportProgress();
    } catch (const yexception& e) {
        IsInReportingProgress = false;
        TRACE_LINE(Log, "");
        TRACE_LINE(Log, "  Exception while processing line: " << data.Sample.TsvLine.FormatErrorPosition());
        throw e;
    }
}

void TDatasetProcessor::OutputSample(const TSampleTsvLine& line, const TSampleProcessingInfo& info) {
    // Prepare for output tsv line with components of current sample.
    // Replace source text by sample text with and without resulting tags.
    TSampleTsvLine lineWithoutTags = line;
    lineWithoutTags[ESampleColumnId::Text] = info.Expected.Text;

    TSampleTsvLine lineWithTags = line;
    TSampleMarkup safeMarkup = info.Actual;
    safeMarkup.MakeSafeForPrint();
    lineWithTags[ESampleColumnId::Text] = safeMarkup.PrintMarkup(Options.SlotPrintingOptions);

    const EResultDatasetType classifierOutput = info.Expected.IsPositive
        ? (info.Actual.IsPositive ? RDT_TRUE_POSITIVE : RDT_FALSE_NEGATIVE)
        : (info.Actual.IsPositive ? RDT_FALSE_POSITIVE : RDT_TRUE_NEGATIVE);
    if (classifierOutput == RDT_FALSE_NEGATIVE) {
        Result.NegativeNgrams.Accumulate(lineWithoutTags[ESampleColumnId::Text]);
    }
    OutputSample(lineWithTags, classifierOutput);

    if (info.Actual.IsPositive) {
        OutputSample(lineWithTags, RDT_RESULT_POSITIVE);
        OutputSample(lineWithoutTags, RDT_RESULT_POSITIVE_DROP_TAGS);
    } else {
        OutputSample(lineWithoutTags, RDT_RESULT_NEGATIVE);
    }

    if (info.Expected.IsPositive && info.Actual.IsPositive) {
        const EResultDatasetType taggerOutput = info.Expected.CheckResult(info.Actual, Options.CompareSlotsByTop)
            ? RDT_TAGGER_CORRECT : RDT_TAGGER_MISMATCH;
        OutputSample(lineWithTags, taggerOutput);
    }

    OutputSample(lineWithTags, RDT_TAGGER_RESULT);
}

void TDatasetProcessor::OutputSample(const TSampleTsvLine& line, EResultDatasetType dataset) {
    if (Writers[dataset].Defined()) {
        Writers[dataset]->WriteLine(line);
    }
}

void TDatasetProcessor::ProcessUnitTest(const TTsvSample& sample, const TSampleProcessingInfo& info,
    const TParserTaskResult::TConstRef& parserResult)
{
    if (!Options.IsVerbose && !Options.IsDebug) {
        return;
    }
    if (info.Expected.CheckResult(info.Actual, Options.CompareSlotsByTop)) {
        return;
    }
    TStringStream out;
    out << "Error:" << Endl;
    out << "  Task:     " << SampleProcessor->GetName() << Endl;
    out << "  Sample:   " << info.Expected.Text << Endl;
    out << "  Expected: " << info.Expected.PrintForReport(Options.SlotPrintingOptions) << Endl;
    out << "  Actual:   " << info.Actual.PrintForReport(Options.SlotPrintingOptions) << Endl;
    out << "  Location: " << sample.TsvLine.FormatErrorPosition() << Endl;
    if (Options.IsDebug && parserResult) {
        parserResult->Dump(&out, "  ");
    }
    Result.UnitTestErrors.push_back(out.Str());
}

void TDatasetProcessor::ReportProgress() {
    ProgressCounter++;
    if (!Log) {
        return;
    }
    if (!IsPowerOf10(ProgressCounter)) {
        return;
    }
    if (IsInReportingProgress) {
        *Log << '0';
        return;
    }
    *Log << "  Progress: " << ProgressCounter;
    IsInReportingProgress = true;
}

void TDatasetProcessor::FinalizeProgress() {
    if (!Log) {
        return;
    }
    *Log << (IsInReportingProgress ? "... total: " : "  Total: ") << ProgressCounter << "." << Endl;
    IsInReportingProgress = false;
}

void TDatasetProcessor::WriteReport() const {
    const TFsPath& dir = Options.ReportDir;
    if (!dir.IsDefined()) {
        return;
    }
    const TFsPath extraDir = dir / "extra";
    extraDir.MkDirs();
    Result.SlowestSamples.Write(extraDir);
    Result.ElementDictionaries.Write(extraDir);
    Result.NegativeNgrams.Write(extraDir);
    if (Options.CollectBlockers) {
        Result.ParserBlockers.Write(dir);
    }
}

void TDatasetProcessor::DumpNotFoundSamples(const TDatasetInfo& dataset, const TVector<TTsvSampleKey>& samples) {
    if (!Log || samples.empty()) {
        return;
    }
    Y_ENSURE(!BaseDatasets.empty());
    IsInReportingProgress = false;
    *Log << Endl;
    *Log << "  Error! " << samples.size() << " samples from " << Cite(dataset.Dataset->GetPath())
        << " not found in base dataset ";
    for (const auto& baseDataset : BaseDatasets) {
        *Log << " " << Cite(baseDataset->GetPath());
    }
    *Log << ":" << Endl;
    for (const auto& [i, sample] : Enumerate(samples)) {
        if (i >= 10) {
            *Log << "     ..." << Endl;
            break;
        }
        *Log << "    " << sample.Reqid << " " << sample.CleanText << Endl;
    }
}

} // namespace NGranet
