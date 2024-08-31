#include "parse_applet.h"
#include "parser.h"
#include "dataset.h"
#include "common.h"
#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/testing/common/env.h>
#include <library/cpp/getopt/small/last_getopt_opts.h>

void DoCase(const TConfigFst& configFst) {
    const TVector<TString> samples = LoadTextLinesFromDataset(BuildRoot() / TFsPath("alice/nlu/data/ru/test/pool") / configFst.DatasetPath);
    TParser parser(configFst);
    TVector<TString> results;

    size_t counter = 0;
    Cout << '[' << configFst.FstName << "]\t";
    for (const auto& sample : samples) {
        const TString parsedString = parser.Parse(sample);
        if (parsedString != "") {
            results.push_back(sample + "\t" + parsedString + "\n");
        }

        if (counter >= (samples.size() / 10)) {
            counter = 0;
            Cout << '*';
            Cout.Flush();
        }
        counter++;
    }
    Cout << Endl;

    SaveTextLinesInTable(configFst.PositiveTablePath, results);
}

int RunParseApplet(int argc, const char** argv) {

    NLastGetopt::TOpts opts = CreateOptions();

    TConfigFst config;
    AddFstConfigOpts(opts, config);

    opts.AddLongOption("dataset", "the path to the dataset")
        .RequiredArgument("dataset")
        .Required()
        .StoreResult(&config.DatasetPath);
        
    opts.AddLongOption("out", "the path to the output table")
        .RequiredArgument("out")
        .Required()
        .StoreResult(&config.PositiveTablePath);
    
    NLastGetopt::TOptsParseResult confingOpts(&opts, argc, argv);

    DoCase(config);
    return 0;
}
