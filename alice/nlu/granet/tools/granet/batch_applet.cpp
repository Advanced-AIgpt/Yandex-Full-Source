#include "batch_applet.h"
#include "common_options.h"
#include "dataset_applet.h"
#include <alice/nlu/granet/lib/compiler/compiler.h>
#include <alice/nlu/granet/lib/grammar/grammar.h>
#include <alice/nlu/granet/lib/test/batch.h>
#include <alice/nlu/granet/lib/utils/trace.h>
#include <dict/dictutil/last_getopt.h>
#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/getopt/modchooser.h>
#include <util/stream/length.h>
#include <util/stream/null.h>
#include <util/stream/zlib.h>
#include <util/string/join.h>
#include <util/string/printf.h>
#include <util/system/hp_timer.h>

namespace NGranet {

static void AddBatchPathOption(NLastGetopt::TOpts* opts, TFsPath* result) {
    Y_ENSURE(opts);
    opts->AddLongOption('b', "batch", "Testing batch directory")
        .Required()
        .RequiredArgument("BATCH_DIR")
        .StoreResult(result);
}

static void AddFilterOption(NLastGetopt::TOpts* opts, TString* result) {
    Y_ENSURE(opts);
    opts->AddLongOption("filter",
        "Perform operation only if name of test case, name of form or name of dataset file contains substring FILTER")
        .Optional()
        .RequiredArgument("FILTER")
        .StoreResult(result);
}

void AddFailedOnlyOption(NLastGetopt::TOpts* opts, bool* result) {
    opts->AddLongOption("failed-only", "Canonize only failed test cases")
        .StoreTrue(result);
}

static TGrammar::TConstRef CompileGrammarWithReport(const TFsPath& grammarPath,
    const TGranetDomain& domain, IOutputStream* log)
{
    TRACE_LINE(log, LogPrefix() << "Grammar info:");
    TRACE_LINE(log, "  Path: " << grammarPath);

    const THPTimer timer;
    TGrammar::TConstRef grammar = NCompiler::TCompiler().CompileFromPath(grammarPath, domain);
    TRACE_LINE(log, "  Compilation time: " << round(timer.Passed() * 1000) << " ms");

    TNullOutput null;
    TCountingOutput compressed(&null);
    TZLibCompress compressor(&compressed, ZLib::GZip);
    TCountingOutput serialized(&compressor);
    grammar->Save(&serialized);
    TRACE_LINE(log, "  Serialized size: " << (serialized.Counter() / 1024) << " KB");
    TRACE_LINE(log, "  Compressed size: " << (compressed.Counter() / 1024) << " KB");
    return grammar;
}

static int RunBatchTest(int argc, const char** argv) {
    NLastGetopt::TOpts opts = CreateOptions();

    NBatch::TBatchProcessor::TOptions options;
    TFsPath grammarPath;

    AddBatchPathOption(&opts, &options.BatchDir);
    AddGrammarPathOption(&opts, &grammarPath);
    AddFilterOption(&opts, &options.Filter);

    opts.AddLongOption("external-result-column",
        "Generate result by external results stored in specified column of base datasets")
        .Optional()
        .RequiredArgument()
        .StoreResult(&options.ExternalResultColumn);

    opts.AddLongOption('o', "output", "Directory for processing info output")
        .Optional()
        .RequiredArgument("OUTPUT_DIR")
        .StoreResult(&options.ResultDir);

    AddOutputColumnFilterOptions(&opts, &options.OutputColumnFilter);

    opts.AddLongOption("consider-slots", "Print tagger result into positive dataset")
        .Optional()
        .RequiredArgument()
        .StoreTrue(&options.NeedSlots);

    AddSampleCacheSizeOption(&opts, &options.SampleCacheSize);

    opts.SetFreeArgsNum(0);
    NLastGetopt::TOptsParseResult(&opts, argc, argv);

    NBatch::TBatchProcessor processor(options, &Cerr);
    TGrammar::TConstRef grammar = CompileGrammarWithReport(grammarPath, processor.GetDomain(), &Cerr);
    processor.Test(grammar);

    Cerr << LogPrefix() << "Metrics:" << Endl;
    PrintTestingReport(processor.GetResultMetrics(), &Cerr);

    const TString utErrors = processor.GetResultUtMessage();
    if (!utErrors.empty()) {
        Cerr << Endl;
        Cerr << utErrors;
    }

    processor.PrintBlockerSuggests(grammar, &Cerr);

    if (options.ResultDir.IsDefined()) {
        Cerr << "More information in " << options.ResultDir << Endl;
    }

    return 0;
}

static int RunBatchCanonize(int argc, const char** argv) {
    NLastGetopt::TOpts opts = CreateOptions();

    NBatch::TBatchProcessor::TOptions options;
    TFsPath grammarPath;
    bool isForFailed = false;

    AddBatchPathOption(&opts, &options.BatchDir);
    AddGrammarPathOption(&opts, &grammarPath);
    AddFilterOption(&opts, &options.Filter);
    AddFailedOnlyOption(&opts, &isForFailed);
    AddOutputColumnFilterOptions(&opts, &options.OutputColumnFilter);
    AddSampleCacheSizeOption(&opts, &options.SampleCacheSize);

    opts.SetFreeArgsNum(0);
    NLastGetopt::TOptsParseResult(&opts, argc, argv);

    NBatch::TBatchProcessor processor(options, &Cerr);
    TGrammar::TConstRef grammar = CompileGrammarWithReport(grammarPath, processor.GetDomain(), &Cerr);
    processor.Canonize(grammar, isForFailed);
    return 0;
}

static int RunBatchCanonizeTagger(int argc, const char** argv) {
    NLastGetopt::TOpts opts = CreateOptions();

    NBatch::TBatchProcessor::TOptions options;
    TFsPath grammarPath;

    AddBatchPathOption(&opts, &options.BatchDir);
    AddGrammarPathOption(&opts, &grammarPath);
    AddFilterOption(&opts, &options.Filter);
    AddSampleCacheSizeOption(&opts, &options.SampleCacheSize);

    opts.SetFreeArgsNum(0);
    NLastGetopt::TOptsParseResult(&opts, argc, argv);

    NBatch::TBatchProcessor processor(options, &Cerr);
    TGrammar::TConstRef grammar = CompileGrammarWithReport(grammarPath, processor.GetDomain(), &Cerr);
    processor.CanonizeTagger(grammar);
    return 0;
}

static int RunBatchUpdateEntities(int argc, const char** argv) {
    NLastGetopt::TOpts opts = CreateOptions();

    NBatch::TBatchProcessor::TOptions options;

    AddBatchPathOption(&opts, &options.BatchDir);
    AddFilterOption(&opts, &options.Filter);

    bool needEmbeddings = false;
    opts.AddLongOption("embeddings", "Fetch embeddings")
        .StoreTrue(&needEmbeddings);

    opts.SetFreeArgsNum(0);
    NLastGetopt::TOptsParseResult(&opts, argc, argv);

    NBatch::TBatchProcessor(options, &Cerr).UpdateEntities(needEmbeddings);

    return 0;
}

int RunBatchApplet(int argc, const char** argv) {
    TModChooser modChooser;

    modChooser.AddMode("test", RunBatchTest, "Run test");

    modChooser.AddMode("canonize", RunBatchCanonize,
        "Canonize \"positive\" datasets. Samples are selected from \"base\" dataset by current grammar.");

    modChooser.AddMode("canonize-tagger", RunBatchCanonizeTagger,
        "Canonize only tagger results for samples of \"positive\" datasets");

    modChooser.AddMode("update-entities", RunBatchUpdateEntities,
        "Fetch and update entities mock in all \"positive\" and \"negative\" datasets");

    return modChooser.Run(argc, argv);
}

} // namespace NGranet
