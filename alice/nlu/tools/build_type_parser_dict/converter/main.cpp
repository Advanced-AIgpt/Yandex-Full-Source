#include <alice/nlu/libs/annotator/annotator_with_mapping.h>
#include <library/cpp/getopt/small/last_getopt.h>
#include <util/folder/path.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/stream/file.h>
#include <util/string/split.h>

struct TOptions {
    TFsPath InputPath;
    TFsPath OutputPath;
};

TOptions ReadOptions(const int argc, const char** argv) {
    TOptions options;

    auto parser = NLastGetopt::TOpts();

    parser.AddLongOption("input")
          .StoreResult(&options.InputPath)
          .Required()
          .Help("tsv-file with format <pattern>\\t<value>");

    parser.AddLongOption("output")
          .StoreResult(&options.OutputPath)
          .Required()
          .Help("Output dictionary path.");

    parser.AddHelpOption();
    parser.SetFreeArgsNum(0);
    NLastGetopt::TOptsParseResult parserResult{&parser, argc, argv};

    return options;
}

void ConvertTsvToDictionary(const TOptions& options) {
    THashMap<TString, THashSet<TString>> mapping;
    TFileInput input(options.InputPath);
    TString line;
    while (input.ReadLine(line)) {
        TVector<TString> strs = StringSplitter(line).Split('\t').Limit(2);
        if (strs.size() != 2) {
            continue;
        }
        mapping[strs[1]].insert(strs[0]);
    }

    NAnnotator::TAnnotatorWithMappingBuilder<TString> builder;
    for (const auto& [key, values] : mapping) {
        for (const auto& value : values) {
            builder.AddPattern(value);
        }
        builder.FinishClass(key);
    }

    TFileOutput output(options.OutputPath);
    builder.Save(&output);
}


int main(const int argc, const char** argv) {
    const auto options = ReadOptions(argc, argv);
    ConvertTsvToDictionary(options);
    return 0;
}
