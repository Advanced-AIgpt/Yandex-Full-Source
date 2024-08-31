#include "debug_applet.h"

#include "common_options.h"
#include "synonyms.h"

#include <alice/nlu/granet/lib/compiler/compiler.h>
#include <alice/nlu/granet/lib/grammar/multi_grammar.h>
#include <alice/nlu/granet/lib/parser/multi_parser.h>
#include <alice/nlu/granet/lib/test/fetcher.h>
#include <alice/nlu/granet/lib/utils/trace.h>
#include <alice/nlu/granet/lib/utils/utils.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/getopt/modchooser.h>

namespace NGranet {

static TVector<TSample::TRef> CreateSamplesFromDatasets(TStringBuf text, const TGranetDomain& domain,
    const TVector<TFsPath>& paths)
{
    TVector<TSample::TRef> samples;
    for (const TFsPath& path : paths) {
        TTsv<ESampleColumnId> tsv;
        tsv.Read(path);
        for (const TTsvLine<ESampleColumnId>& line : tsv.GetLines()) {
            if (text != RemoveTaggerMarkup(line[ESampleColumnId::Text])) {
                continue;
            }
            const TTsvSample tsvSample = TTsvSample::Read(line);
            samples.push_back(CreateSampleFromTsv(tsvSample, domain, EST_TSV | EST_ONLINE));
        }
    }
    return samples;
}

static void FetchEntities(const TGranetDomain& domain, const TBegemotFetcherOptions& fetcherOptions,
    const TSample::TRef& sample, IOutputStream* log)
{
    TSampleMock mock;
    TString fetcherError;
    if (!FetchSampleMock(sample->GetText(), domain, &mock, nullptr, &fetcherError, fetcherOptions)) {
        TRACE_LINE(log, "Error! Can't fetch entities:" << Endl << fetcherError);
        return;
    }
    sample->AddEntitiesOnTokens(mock.Entities);
}

static TVector<TSample::TRef> CreateSamples(TStringBuf text, const TGranetDomain& domain,
    const TBegemotFetcherOptions& fetcherOptions, const TVector<TFsPath>& baseDatasets,
    IOutputStream* log)
{
    if (baseDatasets.empty()) {
        TSample::TRef sample = TSample::Create(text, domain.Lang);
        FetchEntities(domain, fetcherOptions, sample, log);
        return {sample};
    }
    Y_ENSURE(fetcherOptions.Wizextra.empty(), "Option \"wizextra\" conflicts with option \"base\"");
    TVector<TSample::TRef> samples = CreateSamplesFromDatasets(text, domain, baseDatasets);
    if (samples.empty()) {
        TRACE_LINE(log, "Can't find sample " << Cite(text) << " in base dataset.");
    }
    if (samples.size() > 1) {
        TRACE_LINE(log, samples.size() << " samples have been found in base dataset.");
    }
    return samples;
}

static int RunDebugSample(int argc, const char** argv) {
    NLastGetopt::TOpts opts = CreateOptions();

    TString text;
    TGranetDomain domain;
    TBegemotFetcherOptions fetcherOptions;
    TVector<TFsPath> baseDatasets;

    AddTextOption(&opts, &text);
    AddDomainOption(&opts, &domain);
    AddWizextraOption(&opts, &fetcherOptions.Wizextra);
    AddBaseDatasetOption(&opts, &baseDatasets);

    opts.SetFreeArgsNum(0);
    NLastGetopt::TOptsParseResult(&opts, argc, argv);

    for (const TSample::TRef& sample : CreateSamples(text, domain, fetcherOptions, baseDatasets, &Cout)) {
        sample->Dump(&Cout);
    }
    return 0;
}

static int RunDebugParser(int argc, const char** argv) {
    NLastGetopt::TOpts opts = CreateOptions();

    TString text;
    TGranetDomain domain;
    TBegemotFetcherOptions fetcherOptions;
    TVector<TFsPath> baseDatasets;
    TFsPath grammarPath;
    TVector<TFsPath> grammarSourceDirs;
    TString formName;
    TString entityName;
    bool disableParserOptimization = false;
    bool enableParserLog = false;

    AddTextOption(&opts, &text);
    AddDomainOption(&opts, &domain);
    AddWizextraOption(&opts, &fetcherOptions.Wizextra);
    AddBaseDatasetOption(&opts, &baseDatasets);
    AddGrammarPathOption(&opts, &grammarPath);
    AddSourceDirsOption(&opts, &grammarSourceDirs);
    AddFormNameOption(&opts, &formName);
    AddEntityNameOption(&opts, &entityName);
    AddDisableParserOptimizationOption(&opts, &disableParserOptimization);
    AddEnableParserLogOption(&opts, &enableParserLog);

    opts.SetFreeArgsNum(0);
    NLastGetopt::TOptsParseResult(&opts, argc, argv);

    TMultiGrammar::TConstRef grammar = TMultiGrammar::Create(
        NCompiler::TCompiler({.SourceDirs = grammarSourceDirs}).CompileFromPath(grammarPath, domain));

    for (const TSample::TRef& sample : CreateSamples(text, domain, fetcherOptions, baseDatasets, &Cout)) {
        TMultiParser::TRef parser = TMultiParser::Create(grammar, sample, true);
        parser->SetCollectBlockersMode(disableParserOptimization);
        if (enableParserLog) {
            parser->SetLog(&Cout, true);
        }
        TParserTaskResult::TRef result;
        if (!entityName.empty()) {
            result = parser->ParseEntity(entityName);
        } else if (!formName.empty()) {
            result = parser->ParseForm(formName);
        } else {
            Y_ENSURE(false, "Nothing to match. Form or entity name must be set");
        }
        Y_ENSURE(result);
        sample->Dump(&Cout);
        result->Dump(&Cout);
        Cout << "Brief result:" << Endl;
        Cout << "  " << result->ToMarkup().PrintForReport(0) << Endl;
    }
    return 0;
}

static int RunDebugSynonyms(int argc, const char** argv) {
    NLastGetopt::TOpts opts = CreateOptions();

    TGranetDomain domain;
    TFsPath grammarPath;
    TVector<TFsPath> grammarSourceDirs;
    TString formName;
    TString synonymsPath;
    TString synonymsFixlistPath;

    AddGrammarPathOption(&opts, &grammarPath);
    AddSourceDirsOption(&opts, &grammarSourceDirs);
    AddDomainOption(&opts, &domain);
    AddFormNameOption(&opts, &formName);

    opts.AddLongOption("synonyms", "Path to synonyms.gzt.bin")
        .Required()
        .RequiredArgument("SYNONYMS")
        .StoreResult(&synonymsPath);
    opts.AddLongOption("synonyms-fixlist", "Path to fixlist.gzt.bin")
        .Required()
        .RequiredArgument("SYNONYMS")
        .StoreResult(&synonymsFixlistPath);


    opts.SetFreeArgsNum(0);
    NLastGetopt::TOptsParseResult(&opts, argc, argv);

    Y_ENSURE(!formName.empty(), "Nothing to match. Form name must be set");

    THashMap<TString, TSet<TString>> synonyms = LoadSynonyms(synonymsPath, synonymsFixlistPath);
    Cout << "Synonyms: " << synonyms.size() << Endl;

    TMultiGrammar::TRef grammar = TMultiGrammar::Create(
        NCompiler::TCompiler({.SourceDirs = grammarSourceDirs}).CompileFromPath(grammarPath, domain));

    PrintFormSynonyms(*grammar->GetGrammars()[0].Grammar, formName, synonyms, &Cout);

    return 0;
}

int RunDebugApplet(int argc, const char** argv) {
    TModChooser modChooser;

    modChooser.AddMode("sample", RunDebugSample, "Create and dump sample");

    modChooser.AddMode("parser", RunDebugParser, "Parse sample and output parser debug info");

    modChooser.AddMode("synonyms", RunDebugSynonyms, "Synonyms debug info");

    return modChooser.Run(argc, argv);
}

} // namespace NGranet
