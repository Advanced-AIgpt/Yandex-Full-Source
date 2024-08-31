#include "dataset_applet.h"
#include "common_options.h"
#include <alice/nlu/granet/lib/utils/utils.h>
#include <alice/nlu/granet/lib/grammar/grammar.h>
#include <alice/nlu/granet/lib/test/dataset.h>
#include <alice/nlu/granet/lib/test/dataset_builder.h>
#include <alice/nlu/granet/lib/test/dataset_mock_updater.h>
#include <alice/nlu/granet/lib/test/dataset_processor.h>
#include <dict/dictutil/last_getopt.h>
#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/getopt/modchooser.h>

namespace NGranet {

// ~~~~ Applets ~~~~

static int RunDatasetCreate(int argc, const char** argv) {
    NLastGetopt::TOpts opts = CreateOptions();

    TDatasetBuilder::TOptions options;
    TGranetDomain domain;

    opts.AddLongOption('i', "input", "Input txt or tsv file")
        .Required()
        .RequiredArgument("INPUT")
        .StoreResult(&options.InputPath);

    opts.AddLongOption('o', "output", "Result dataset (tsv-file)")
        .Required()
        .RequiredArgument("OUTPUT")
        .StoreResult(&options.OutputPath);

    opts.AddLongOption("context", "Path to result context storage (json file with parts of contexts)")
        .Optional()
        .RequiredArgument()
        .StoreResult(&options.ContextStoragePath);

    opts.AddLongOption("columns", "Comma-separated list of result columns")
        .Optional()
        .RequiredArgument()
        .StoreResult(&options.Columns);

    opts.AddLongOption("limit", "Unique sample count limit")
        .Optional()
        .RequiredArgument()
        .StoreResult(&options.SampleCountLimit);

    opts.AddLongOption("merge-duplicated", "Merge duplicated samples")
        .StoreTrue(&options.MergeDuplicated);

    opts.AddLongOption("keep-tags", "Keep tagger markup")
        .StoreTrue(&options.KeepTags);

    opts.AddLongOption("normalize", "Normalize utterances: remove punctuation, convert to lower case, etc.")
        .StoreTrue(&options.ShouldNormalize);

    opts.AddLongOption("lower-case", "Convert to lower case (proper language needed)")
        .StoreTrue(&options.ToLowerCase);

    bool noEntities = false;
    opts.AddLongOption("no-entities", "Don't fetch entities")
        .StoreTrue(&noEntities);

    bool needEmbeddings = false;
    opts.AddLongOption("embeddings", "Fetch embeddings")
        .StoreTrue(&needEmbeddings);

    AddDomainOption(&opts, &domain);

    opts.SetFreeArgsNum(0);
    NLastGetopt::TOptsParseResult(&opts, argc, argv);

    // Create dataset.
    options.Lang = domain.Lang;
    TDatasetBuilder(options, &Cerr).BuildDataset();

    // Fetch entities and embeddings.
    if (!noEntities || needEmbeddings) {
        const TDatasetMockUpdater::TOptions fetcherOptions = {
            .SrcPath = options.OutputPath,
            .Domain = domain,
            .ShouldUpdateMock = !noEntities,
            .ShouldUpdateEmbeddings = needEmbeddings,
            .RemoveBadSamples = true
        };
        TDatasetMockUpdater(fetcherOptions, &Cerr).Update();
    }

    return 0;
}

static int RunDatasetUpdateEntities(int argc, const char** argv) {
    NLastGetopt::TOpts opts = CreateOptions();
    TDatasetMockUpdater::TOptions options;

    TFsPath inputPath;
    opts.AddLongOption('i', "input", "Input dataset (tsv-file)")
        .Required()
        .RequiredArgument("INPUT")
        .StoreResult(&options.SrcPath);

    TFsPath outputPath;
    opts.AddLongOption('o', "output", "Result dataset (tsv-file). If not defined INPUT is used as OUTPUT")
        .Optional()
        .RequiredArgument("OUTPUT")
        .StoreResult(&options.DestPath);

    opts.AddLongOption("embeddings", "Fetch embeddings")
        .StoreTrue(&options.ShouldUpdateEmbeddings);

    opts.AddLongOption("missing", "Fetch only missing mocks")
        .StoreTrue(&options.OnlyMissing);

    opts.AddLongOption("remove-bad-samples", "Remove samples which cause empty request or fetcher error")
        .StoreTrue(&options.RemoveBadSamples);

    AddDomainOption(&opts, &options.Domain);

    opts.SetFreeArgsNum(0);
    NLastGetopt::TOptsParseResult(&opts, argc, argv);

    TDatasetMockUpdater(options, &Cerr).Update();

    return 0;
}

static int RunDatasetSelect(int argc, const char** argv) {
    NLastGetopt::TOpts opts = CreateOptions();

    TSampleProcessorByGrammar::TOptions processingOptions;
    TDatasetProcessorOptions options;
    TFsPath grammarPath;
    TVector<TFsPath> grammarSourceDirs;

    opts.AddLongOption('i', "input", "Input dataset (tsv-file)")
        .Required()
        .RequiredArgument("INPUT")
        .AppendTo(&options.PositiveTruth);

    AddBaseDatasetOption(&opts, &options.BaseDatasets);

    opts.AddLongOption("ignore", "Dataset with samples excluded from input dataset")
        .Optional()
        .RequiredArgument()
        .AppendTo(&options.IgnoredSamples);

    AddGrammarPathOption(&opts, &grammarPath);
    AddSourceDirsOption(&opts, &grammarSourceDirs);
    AddDomainOption(&opts, &processingOptions.Domain);
    AddFormNameOption(&opts, &processingOptions.TaskName);
    AddEntitySourceOptions(&opts, &processingOptions.EntitySources);

    opts.AddLongOption('p', "positive", "Result dataset with positive samples (tsv-file)")
        .Optional()
        .RequiredArgument()
        .StoreResult(&options.ResultDatasets[RDT_RESULT_POSITIVE]);

    opts.AddLongOption('n', "negative", "Result dataset with negative samples (tsv-file)")
        .Optional()
        .RequiredArgument()
        .StoreResult(&options.ResultDatasets[RDT_FALSE_NEGATIVE]);

    AddReportDirOption(&opts, &options.ReportDir);
    AddCollectBlockersOption(&opts, &options.CollectBlockers);
    AddOutputColumnFilterOptions(&opts, &options.OutputColumnFilter);
    AddSlotComponentsOptions(&opts, &options);

    opts.SetFreeArgsNum(0);
    NLastGetopt::TOptsParseResult(&opts, argc, argv);

    Cerr << "Processing:" << Endl;
    processingOptions.CollectBlockers = options.CollectBlockers;
    TSampleProcessor::TRef sampleProcessor = MakeIntrusive<TSampleProcessorByGrammar>(processingOptions,
        grammarPath, grammarSourceDirs);
    const TDatasetProcessorResult result = TDatasetProcessor(options, sampleProcessor, nullptr, &Cerr).Process();
    PrintSelectionReport(result.Metrics, "Result:", &Cerr);

    return 0;
}

static int RunDatasetTest(int argc, const char** argv) {
    NLastGetopt::TOpts opts = CreateOptions();

    TSampleProcessorByGrammar::TOptions processingOptions;
    TDatasetProcessorOptions options;
    TFsPath grammarPath;
    TVector<TFsPath> grammarSourceDirs;

    opts.AddLongOption('p', "positive", "Input dataset with positive samples (tsv-file)")
        .Optional()
        .RequiredArgument()
        .AppendTo(&options.PositiveTruth);

    opts.AddLongOption('n', "negative", "Input dataset with negative samples (tsv-file)")
        .Optional()
        .RequiredArgument()
        .AppendTo(&options.NegativeTruth);

    AddBaseDatasetOption(&opts, &options.BaseDatasets);

    opts.AddLongOption("ignore", "Dataset with samples excluded from input dataset")
        .Optional()
        .RequiredArgument()
        .AppendTo(&options.IgnoredSamples);

    AddGrammarPathOption(&opts, &grammarPath);
    AddSourceDirsOption(&opts, &grammarSourceDirs);
    AddDomainOption(&opts, &processingOptions.Domain);
    AddFormNameOption(&opts, &processingOptions.TaskName);
    AddEntitySourceOptions(&opts, &processingOptions.EntitySources);

    opts.AddLongOption("false-positive", "Output dataset for false positive samples")
        .AddLongName("fp")
        .Optional()
        .RequiredArgument()
        .StoreResult(&options.ResultDatasets[RDT_FALSE_POSITIVE]);

    opts.AddLongOption("false-negative", "Output dataset for false negative samples")
        .AddLongName("fn")
        .Optional()
        .RequiredArgument()
        .StoreResult(&options.ResultDatasets[RDT_FALSE_NEGATIVE]);

    opts.AddLongOption("true-positive", "Output dataset for true positive samples")
        .AddLongName("tp")
        .Optional()
        .RequiredArgument()
        .StoreResult(&options.ResultDatasets[RDT_TRUE_POSITIVE]);

    opts.AddLongOption("true-negative", "Output dataset for true negative samples")
        .AddLongName("tn")
        .Optional()
        .RequiredArgument()
        .StoreResult(&options.ResultDatasets[RDT_TRUE_NEGATIVE]);

    opts.AddLongOption("tagger-mismatch", "Output dataset for true positive samples with tagger error")
        .Optional()
        .RequiredArgument()
        .StoreResult(&options.ResultDatasets[RDT_TAGGER_MISMATCH]);

    AddReportDirOption(&opts, &options.ReportDir);
    AddCollectBlockersOption(&opts, &options.CollectBlockers);
    AddOutputColumnFilterOptions(&opts, &options.OutputColumnFilter);
    AddSlotComponentsOptions(&opts, &options);

    opts.SetFreeArgsNum(0);
    NLastGetopt::TOptsParseResult(&opts, argc, argv);

    Cerr << "Processing:" << Endl;
    processingOptions.CollectBlockers = options.CollectBlockers;
    TSampleProcessor::TRef sampleProcessor = MakeIntrusive<TSampleProcessorByGrammar>(processingOptions,
        grammarPath, grammarSourceDirs);
    const TDatasetProcessorResult result = TDatasetProcessor(options, sampleProcessor, nullptr, &Cerr).Process();
    PrintTestingReport(result.Metrics, "Result:", true, &Cerr);

    return 0;
}

static int RunDatasetFilter(int argc, const char** argv) {
    NLastGetopt::TOpts opts = CreateOptions();

    TDatasetProcessorOptions options;
    options.OutputColumnFilter = SCF_ALL_COLUMNS;

    opts.AddLongOption('b', "base", "Base dataset used as source of entities")
        .Required()
        .RequiredArgument()
        .AppendTo(&options.BaseDatasets);

    opts.AddLongOption('i', "input", "Input dataset used as filter for samples selection from base dataset")
        .Required()
        .RequiredArgument("INPUT")
        .AppendTo(&options.PositiveTruth);

    opts.AddLongOption('o', "output", "Result subset of base dataset")
        .Required()
        .RequiredArgument("OUTPUT")
        .StoreResult(&options.ResultDatasets[RDT_TRUE_POSITIVE]);

    opts.AddLongOption("ignore", "Dataset with samples excluded from input dataset")
        .Optional()
        .RequiredArgument()
        .AppendTo(&options.IgnoredSamples);

    AddOutputColumnFilterOptions(&opts, &options.OutputColumnFilter);
    AddSlotComponentsOptions(&opts, &options);

    opts.SetFreeArgsNum(0);
    NLastGetopt::TOptsParseResult(&opts, argc, argv);

    Cerr << "Processing:" << Endl;
    TSampleProcessor::TRef sampleProcessor = MakeIntrusive<TSampleProcessorAlwaysTrue>();
    const TDatasetProcessorResult result = TDatasetProcessor(options, sampleProcessor, nullptr, &Cerr).Process();
    PrintSelectionReport(result.Metrics, "Result:", &Cerr);

    return 0;
}

int RunDatasetApplet(int argc, const char** argv) {
    TModChooser modChooser;

    modChooser.AddMode("create", RunDatasetCreate, "Create dataset from text or tsv file");

    modChooser.AddMode("update-entities", RunDatasetUpdateEntities,
        "Fetch entities from Begemot and update entities mock in dataset. "
        "Begemot url can be specified by environment variable GRANET_BEGEMOT. "
        "Default url is hamzard.yandex.net:8891");

    modChooser.AddMode("select", RunDatasetSelect, "Select subset of dataset by grammar");

    modChooser.AddMode("test", RunDatasetTest, "Test grammar on dataset");

    modChooser.AddMode("filter", RunDatasetFilter, "To be commented...");

    return modChooser.Run(argc, argv);
}

} // namespace NGranet
