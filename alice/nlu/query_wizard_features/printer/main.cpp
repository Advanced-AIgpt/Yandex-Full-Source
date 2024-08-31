#include <alice/nlu/query_wizard_features/reader/reader.h>
#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/streams/factory/factory.h>


int main(int argc, const char** argv) {
    NLastGetopt::TOpts opts;

    TString prefix;
    TString output;

    opts.AddLongOption("prefix", "prefix name for .trie and .data files")
        .DefaultValue("query_wizard_features")
        .StoreResult(&prefix);

    opts.AddLongOption("out", "output file")
        .DefaultValue("-")
        .StoreResult(&output);

    NLastGetopt::TOptsParseResult res(&opts, argc, argv);

    auto out = OpenOutput(output);
    Y_ENSURE(out, "no out!");
    NQueryWizardFeatures::TReader reader;
    reader.Load(prefix + ".trie", prefix + ".data");
    reader.Dump(*out);
}

