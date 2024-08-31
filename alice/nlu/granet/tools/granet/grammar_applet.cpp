#include "grammar_applet.h"
#include "common_options.h"
#include <alice/nlu/granet/lib/compiler/compiler.h>
#include <alice/nlu/granet/lib/compiler/source_text_collection.h>
#include <dict/dictutil/last_getopt.h>
#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/getopt/modchooser.h>

using namespace std::literals;

namespace NGranet {

static int RunPackGrammarApplet(int argc, const char** argv) {
    NLastGetopt::TOpts opts = CreateOptions();

    TFsPath grammarPath;
    AddGrammarPathOption(&opts, &grammarPath);

    TVector<TFsPath> grammarSourceDirs;
    AddSourceDirsOption(&opts, &grammarSourceDirs);

    TGranetDomain domain;
    AddDomainOption(&opts, &domain);

    bool isJson = false;
    opts.AddLongOption("json", "Print collected sources as json").StoreTrue(&isJson);

    bool isJsonPretty = false;
    opts.AddLongOption("json-pretty", "Print collected sources as prettified json").StoreTrue(&isJsonPretty);

    opts.SetFreeArgsNum(0);
    NLastGetopt::TOptsParseResult(&opts, argc, argv);

    NCompiler::TCompiler compiler({.SourceDirs = grammarSourceDirs});
    const NCompiler::TSourceTextCollection collection = compiler.CollectSourceTexts(grammarPath, domain);
    if (isJsonPretty) {
        Cout << collection.ToTValue().ToJsonPretty() << '\n';
    } else if (isJson) {
        Cout << collection.ToTValue().ToJson();
    } else {
        Cout << collection.ToCompressedBase64();
    }
    return 0;
}

static int RunUnpackGrammarApplet(int argc, const char** argv) {
    NLastGetopt::TOpts opts = CreateOptions();
    opts.SetFreeArgsNum(0);
    NLastGetopt::TOptsParseResult(&opts, argc, argv);

    NCompiler::TSourceTextCollection collection;
    collection.FromCompressedBase64(Cin.ReadAll());
    Cout << collection.ToTValue().ToJsonPretty() << '\n';
    return 0;
}

static void ReadWordTypesArgument(const TVector<TString>& types, bool* needExact, bool* needLemma) {
    Y_ENSURE(needExact);
    Y_ENSURE(needLemma);
    for (const TString& type : types) {
        if (type == "all"sv) {
            *needExact = true;
            *needLemma = true;
        } else if (type == "exact"sv) {
            *needExact = true;
        } else if (type == "lemma"sv) {
            *needLemma = true;
        } else {
            Y_ENSURE(false, "Unknown type of word: " + type);
        }
    }
}

static int RunPrintWordsGrammarApplet(int argc, const char** argv) {
    NLastGetopt::TOpts opts = CreateOptions();

    TFsPath grammarPath;
    AddGrammarPathOption(&opts, &grammarPath);

    NCompiler::TCompilerOptions compilerOptions;
    AddSourceDirsOption(&opts, &compilerOptions.SourceDirs);

    TGranetDomain domain;
    AddDomainOption(&opts, &domain);

    TVector<TString> types;
    opts.AddLongOption("types", "Comma-separated list of types of words. Available types: all, exact, lemma")
        .DefaultValue("all")
        .SplitHandler(&types, ',');

    opts.SetFreeArgsNum(0);
    NLastGetopt::TOptsParseResult(&opts, argc, argv);

    bool needExact = false;
    bool needLemma = false;
    ReadWordTypesArgument(types, &needExact, &needLemma);

    TGrammar::TConstRef grammar = NCompiler::TCompiler(compilerOptions).CompileFromPath(grammarPath, domain);
    grammar->PrintWords(&Cout, needExact, needLemma);

    return 0;
}

int RunGrammarApplet(int argc, const char** argv) {
    TModChooser modChooser;

    modChooser.AddMode("pack", RunPackGrammarApplet,
        "Collect source files needed to compile grammar and print them as Begemot CGI-parameter");

    modChooser.AddMode("unpack", RunUnpackGrammarApplet,
        "Read from stdin grammar in CGI-parameter-form and convert it to json.");

    modChooser.AddMode("print_words", RunPrintWordsGrammarApplet,
        "Print words used in grammar");

    return modChooser.Run(argc, argv);
}

} // namespace NGranet
