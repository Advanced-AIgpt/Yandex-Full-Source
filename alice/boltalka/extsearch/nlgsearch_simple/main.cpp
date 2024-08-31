#include <alice/boltalka/libs/nlgsearch_simple/nlgsearch_simple.h>

#include <library/cpp/getopt/last_getopt.h>

#include <util/stream/output.h>

#include <contrib/libs/intel/mkl/include/mkl.h>

void WriteNlgSearchOutput(const NNlg::TNlgSearchSimple& nlgsearcher, IInputStream& in, IOutputStream& out) {
    TString context;
    while (in.ReadLine(context)) {
        TVector<TString> contextSplitted = StringSplitter(context).Split('\t');
        auto searchResults = nlgsearcher.GetSearchResults(contextSplitted);
        for (const auto& searchResult : searchResults) {
            out << TRelevance(1e9 * searchResult.Score) << "\t" << context << "\t" << searchResult.Reply << "\n";
        }
    }
}

int main(int argc, const char** argv) {
    mkl_cbwr_set(MKL_CBWR_COMPATIBLE);

    TString inputFile;
    TString outputFile;
    TString indexDir;
    TString memoryMode;
    TString dssmModelNames;
    TString baseDssmModelName;
    TString factorDssmModelNames;
    TString rerankerModelNamesToLoad;
    TString rerankerModelName;
    TString knnIndexNames;
    TString baseKnnIndexName;
    TString seq2seqExternalUri;
    TString numStaticFactors;
    size_t maxResults;
    TString experiments;

    NLastGetopt::TOpts opts = NLastGetopt::TOpts::Default();
    opts
        .AddHelpOption();
    opts
        .AddLongOption('i', "input")
        .RequiredArgument("FILE")
        .Required()
        .StoreResult(&inputFile);
    opts
        .AddLongOption('o', "output")
        .RequiredArgument("FILE")
        .Required()
        .StoreResult(&outputFile);
    opts
        .AddLongOption("data")
        .Required()
        .StoreResult(&indexDir);
    opts
        .AddLongOption("memory-mode")
        .DefaultValue("Precharged")
        .StoreResult(&memoryMode);
    opts
        .AddLongOption("dssms")
        .Required()
        .StoreResult(&dssmModelNames);
    opts
        .AddLongOption("knn-indexes")
        .Required()
        .StoreResult(&knnIndexNames);
    opts
        .AddLongOption("base-dssm")
        .DefaultValue("")
        .StoreResult(&baseDssmModelName);
    opts
        .AddLongOption("factor-dssms")
        .DefaultValue("")
        .StoreResult(&factorDssmModelNames);
    opts
        .AddLongOption("rerankers-to-load")
        .DefaultValue("")
        .StoreResult(&rerankerModelNamesToLoad);
    opts
        .AddLongOption("reranker")
        .DefaultValue("")
        .StoreResult(&rerankerModelName);
    opts
        .AddLongOption("base-knn-index")
        .DefaultValue("")
        .StoreResult(&baseKnnIndexName);
    opts
        .AddLongOption("seq2seq-external-uri")
        .DefaultValue("")
        .StoreResult(&seq2seqExternalUri);
    opts
        .AddLongOption("max-results")
        .RequiredArgument("INT")
        .DefaultValue(10)
        .StoreResult(&maxResults);
    opts
        .AddLongOption("experiments")
        .DefaultValue("")
        .StoreResult(&experiments);

    opts.SetFreeArgsNum(0);
    opts.AddHelpOption('h');
    NLastGetopt::TOptsParseResult res(&opts, argc, argv);

    NNlg::TNlgSearchSimpleParams params = {
        indexDir, memoryMode, dssmModelNames, baseDssmModelName, factorDssmModelNames, rerankerModelNamesToLoad, rerankerModelName, knnIndexNames, baseKnnIndexName, seq2seqExternalUri, maxResults, experiments, 1
    };
    NNlg::TNlgSearchSimple nlgsearcher(params);
    TFileInput in(inputFile);
    TFileOutput out(outputFile);
    WriteNlgSearchOutput(nlgsearcher, in, out);
    return 0;
}
