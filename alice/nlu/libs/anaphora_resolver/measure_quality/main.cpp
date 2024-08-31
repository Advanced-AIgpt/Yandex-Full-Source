#include <alice/nlu/libs/anaphora_resolver/measure_quality/lib/measure_quality.h>
#include <library/cpp/getopt/small/last_getopt.h>
#include <util/generic/serialized_enum.h>
#include <util/stream/null.h>

NAlice::TMeasureAnaphoraMatcherQualityOptions ReadOptions(const int argc, const char** argv) {
    NAlice::TMeasureAnaphoraMatcherQualityOptions result;

    auto parser = NLastGetopt::TOpts();
    parser.AddLongOption("model-dir")
          .StoreResult(&result.ModelDirPath)
          .Required()
          .Help("Model directory path.");

    parser.AddLongOption("embedder-dir")
          .StoreResult(&result.EmbedderDirPath)
          .Required()
          .Help("Embedder directory path.");

    parser.AddLongOption("test-data")
          .StoreResult(&result.TestDataPath)
          .Required()
          .Help("Test data file path.");

    parser.AddLongOption("results")
          .StoreResult(&result.ResultsPath)
          .Required()
          .Help("File to store results path.");

    parser.AddLongOption("model-type")
          .RequiredArgument("ENUM")
          .StoreResult(&result.MatcherModel)
          .Required()
          .Help("Matcher model type of: " + GetEnumAllNames<NAlice::EAnaphoraMatcherModel>());

    parser.AddLongOption("verbose")
          .StoreTrue(&result.Verbose)
          .Help("Log to stdout.");

    parser.AddHelpOption();
    parser.SetFreeArgsNum(0);
    NLastGetopt::TOptsParseResult parserResult{&parser, argc, argv};

    return result;
}

int main(const int argc, const char** argv) {
    const auto options = ReadOptions(argc, argv);

    if (options.Verbose) {
        NAlice::MeasureAnaphoraMatcherQuality(options, &Cout);
    } else {
        TNullOutput nullOutput;
        NAlice::MeasureAnaphoraMatcherQuality(options, &nullOutput);
    }

    return 0;
}
