#include "normalizer.h"
#include "common_options.h"
#include <alice/nlu/libs/request_normalizer/request_normalizer.h>
#include <alice/nlu/libs/tokenization/tokenizer.h>
#include <dict/dictutil/dictutil.h>
#include <dict/dictutil/last_getopt.h>
#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/langs/langs.h>
#include <util/folder/path.h>
#include <util/stream/file.h>
#include <util/stream/input.h>
#include <util/stream/output.h>
#include <util/string/builder.h>
#include <util/string/join.h>
#include <util/string/vector.h>

namespace NGranet {

static TString Normalize(const TString& line, ELanguage language, const TString& type) {
    TString normalized;
    if (type == "original") {
        return line;
    } else if (type == "basic") {
        const TString lowerCasedLine = WideToUTF8(ToLower(language, UTF8ToWide(line)));
        return JoinSeq(" ", NNlu::TSmartTokenizer(lowerCasedLine, language).GetOriginalTokens());
    } else if (type == "granet") {
        return JoinSeq(" ", NNlu::TSmartTokenizer(line, language).GetNormalizedTokens());
    } else if (type == "fst") {
        return NNlu::TRequestNormalizer::Normalize(language, line);
    }
    Y_ENSURE(false, "Unknown type of normalization: " + type);
    return "";
}

int RunNormalizeApplet(int argc, const char *argv[]) {
    NLastGetopt::TOpts opts = CreateOptions();

    TFsPath inputPath;
    opts.AddLongOption('i', "input", "Input file path. Default is stdin.")
        .RequiredArgument("INPUT")
        .StoreResult(&inputPath);

    TFsPath outputPath;
    opts.AddLongOption('o', "output", "Output file path. Default is stdout.")
        .RequiredArgument("OUTPUT")
        .StoreResult(&outputPath);

    ELanguage language = LANG_RUS;
    AddLanguageOption(&opts, &language);

    TVector<TString> types;
    opts.AddLongOption("type",
            "Comma-separated list of result normalizations. Available types:\n"
            "  - 'original' - original text.\n"
            "  - 'basic' - tokenized, lower case.\n"
            "  - 'granet' - tokenized, NormalizeUnicode (joined NGranet::TSample::GetTokens).\n"
            "  - 'fst' - tokenized, numbers written in digits (fst_normalizer).\n")
        .DefaultValue("tokens")
        .SplitHandler(&types, ',');

    size_t columnIndex = NPOS;
    opts.AddLongOption("column",
            "Index of normalized column of tab-separated file.\n"
            "By default whole line normalized.")
        .StoreResult(&columnIndex);

    bool hasHeader = false;
    opts.AddLongOption("header", "First line is header of tsv-file.\n")
        .StoreTrue(&hasHeader);

    bool shouldFilter = false;
    opts.AddLongOption("filter", "Remove duplicated and empty results.\n")
        .StoreTrue(&shouldFilter);

    NLastGetopt::TOptsParseResult config(&opts, argc, argv);

    // Prepare input
    IInputStream* input = &Cin;
    THolder<TFileInput> inputFile;
    if (inputPath.IsDefined()) {
        inputFile = MakeHolder<TFileInput>(inputPath);
        input = inputFile.Get();
    }

    // Prepare output
    IOutputStream* output = &Cout;
    THolder<TFileOutput> outputFile;
    if (outputPath.IsDefined()) {
        outputFile = MakeHolder<TFileOutput>(outputPath);
        output = outputFile.Get();
    }

    // Process header of tsv-file
    TString line;
    if (hasHeader && input->ReadLine(line)) {
        Y_ENSURE(columnIndex != NPOS, "--header option allowed only if --column defined.");
        TVector<TString> cells = SplitString(line, "\t");
        if (columnIndex < cells.size()) {
            cells.erase(cells.begin() + columnIndex);
            cells.insert(cells.begin() + columnIndex, types.begin(), types.end());
        }
        *output << JoinSeq("\t", cells) << '\n';
    }

    // Process lines
    THashSet<TString> history;
    while (input->ReadLine(line)) {
        TVector<TString> cells;
        size_t insertPos = 0;
        if (columnIndex != NPOS) {
            cells = SplitString(line, "\t");
            if (columnIndex >= cells.size()) {
                if (!shouldFilter) {
                    *output << line << '\n';
                }
                continue;
            }
            line = cells[columnIndex];
            cells.erase(cells.begin() + columnIndex);
            insertPos = columnIndex;
        }

        bool hasEmpty = false;
        TStringBuilder merged;
        for (const TString& type: types) {
            const TString normalized = Normalize(line, language, type);
            hasEmpty = hasEmpty || normalized.empty();
            merged << (!merged.empty() ? "\t" : "") << normalized;
        }

        if (shouldFilter) {
            const TString basic = Normalize(line, language, "basic");
            if (hasEmpty || !history.insert(basic).second) {
                continue;
            }
        }

        cells.insert(cells.begin() + insertPos, merged);
        *output << JoinSeq("\t", cells) << '\n';
    }
    return 0;
}

} // namespace NGranet
