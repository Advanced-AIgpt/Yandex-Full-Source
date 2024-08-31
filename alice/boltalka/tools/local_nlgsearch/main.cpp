#include <alice/boltalka/extsearch/base/search/search.h>

#include <search/config/virthost.h>
#include <ysite/yandex/srchmngr/arcmgr.h>
#include <search/reqparam/reqparam.h>

#include <util/string/util.h>
#include <util/string/join.h>
#include <util/string/subst.h>
#include <util/folder/path.h>

#include <library/cpp/getopt/last_getopt.h>


int main(int argc, char** argv) {
    TString indexDir;
    TSearchConfig searchConfig;
    // Disable logging for local nlgsearch.
    searchConfig.UserDirectives["LoggingQueueSize"] = "0";

    NLastGetopt::TOpts opts = NLastGetopt::TOpts::Default();
    opts
        .AddHelpOption('h');
    opts
        .AddLongOption("index-dir")
        .Required()
        .Help("Path to index directory.")
        .StoreResult(&indexDir);
    opts
        .AddLongOption("search-for")
        .OptionalArgument()
        .DefaultValue("context_and_reply")
        .Handler1T<TString>([&](const TString& value) {
            searchConfig.UserDirectives["SearchFor"] = value;
        });
    opts
        .AddLongOption("search-by")
        .OptionalArgument()
        .DefaultValue("context")
        .Handler1T<TString>([&](const TString& value) {
            searchConfig.UserDirectives["SearchBy"] = value;
        });
    opts
        .AddLongOption("max-results")
        .OptionalArgument()
        .DefaultValue("1")
        .Handler1T<TString>([&](const TString& value) {
            searchConfig.UserDirectives["MaxResults"] = value;
        })
        .Help("Number of replies to take from top.");
    opts
        .AddLongOption("dssm-model-name")
        .OptionalArgument()
        .DefaultValue("insight_c3_rus_lister")
        .Handler1T<TString>([&](const TString& value) {
            searchConfig.UserDirectives["DssmModelName"] = value;
            searchConfig.UserDirectives["DssmModelsToLoad"] = value;
        });
    opts
        .AddLongOption("factor-dssm-models-to-load")
        .OptionalArgument()
        .Handler1T<TString>([&](const TString& value) {
            searchConfig.UserDirectives["FactorDssmModelsToLoad"] = value;
        });
    opts
        .AddLongOption("ranker-model-name")
        .OptionalArgument()
        .Handler1T<TString>([&](const TString& value) {
            searchConfig.UserDirectives["RankerModelsToLoad"] = value;
            searchConfig.UserDirectives["RankerModelName"] = value;
        });
    opts.SetFreeArgsNum(0);

    NLastGetopt::TOptsParseResult args(&opts, argc, argv);

    TArchiveManager archiveManager(TFsPath(indexDir) / "index");
    Cerr << "Loaded archive manager." << Endl;

    NNlg::TNlgSearchPtr searcher = NNlg::CreateNlgSearch(indexDir, searchConfig);

    Cerr << "Start processing contexts." << Endl;
    TRequestParams stubParams;
    NNlg::TNlgRelevancePtr relevance = searcher->CreateRelevanceDeprecated(archiveManager, &stubParams);
    for (TString context; Cin.ReadLine(context);) {
        SubstGlobal(context, "\\t", "\n");
        auto results = relevance->CalcFilteringResultsForQuery(context);
        SubstGlobal(context, "\n", "\t");
        for (size_t i = 0; i < results.Docs.size(); ++i) {
            Cout << (i64) (results.Docs[i].Score * 1e9) << "\t" << context << "\t" << results.Replies[i] << Endl;
        }
    }
    return 0;
}
