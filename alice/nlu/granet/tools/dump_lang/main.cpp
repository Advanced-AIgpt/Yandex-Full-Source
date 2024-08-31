#include "dump_granet_inflector.h"
#include "dump_lemmer.h"
#include "dump_normalizer.h"
#include "dump_tokenizer.h"
#include <dict/dictutil/last_getopt.h>
#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/getopt/modchooser.h>
#include <library/cpp/langmask/langmask.h>
#include <util/string/strip.h>

namespace {

    void ReadArguments(int argc, const char** argv, TString* text, TLangMask* langs,
        TString* grams, bool* isVerbose)
    {
        TString langNames = "ru";

        NLastGetopt::TOpts opts = NLastGetopt::TOpts::Default();
        opts.AddHelpOption('h');
        if (langs != nullptr) {
            opts.AddLongOption("lang", "Comma-separated list of languages").StoreResult(&langNames);
        }
        if (text != nullptr) {
            opts.AddLongOption("text", "Processed text. If not defined stdin is used").StoreResult(text);
        }
        if (grams != nullptr) {
            opts.AddLongOption("grams", "Destination grammemes for inflector").StoreResult(grams);
        }
        if (isVerbose != nullptr) {
            opts.AddLongOption('v', "verbose", "Be verbose").StoreTrue(isVerbose);
        }
        opts.SetFreeArgsNum(0);
        NLastGetopt::TOptsParseResult(&opts, argc, argv);

        if (langs != nullptr) {
            for (const TStringBuf langName : StringSplitter(langNames).Split(',').SkipEmpty()) {
                *langs |= LanguageByNameOrDie(langName);
            }
        }
    }

    int RunNormalizerApplet(int argc, const char** argv) {
        TLangMask langs;
        TString text;
        ReadArguments(argc, argv, &text, &langs, nullptr, nullptr);

        IInputStream* in = &Cin;
        TStringInput textInput(text);
        if (!text.empty()) {
            in = &textInput;
        }

        TUtf16String line;
        while (in->ReadLine(line)) {
            for (TWtringBuf word : StringSplitter(line).Split(u' ')) {
                word = StripString(word);
                if (word.empty()) {
                    continue;
                }
                DumpNormalizer(word, langs, &Cout);
            }
        }
        return 0;
    }

    int RunLemmerApplet(int argc, const char** argv) {
        TLangMask langs;
        TString text;
        bool isVerbose = false;
        ReadArguments(argc, argv, &text, &langs, nullptr, &isVerbose);

        IInputStream* in = &Cin;
        TStringInput textInput(text);
        if (!text.empty()) {
            in = &textInput;
        }

        TUtf16String line;
        while (in->ReadLine(line)) {
            for (TWtringBuf word : StringSplitter(line).Split(u' ')) {
                word = StripString(word);
                if (word.empty()) {
                    continue;
                }
                DumpLemmer(word, langs, isVerbose, &Cout);
            }
        }
        return 0;
    }

    static int RunTokenizerApplet(int argc, const char** argv) {
        TLangMask langs;
        TString text;
        ReadArguments(argc, argv, &text, &langs, nullptr, nullptr);

        IInputStream* in = &Cin;
        TStringInput textInput(text);
        if (!text.empty()) {
            in = &textInput;
        }

        TUtf16String line;
        while (in->ReadLine(line)) {
            const TTokenizerOptions options = {
                .SpacePreserve = false,
                .LangMask = langs,
                .UrlDecode = true,
                .KeepAffixes = false,
            };
            TNlpTokenizerDumper(&Cout).Tokenize(line, options);
        }
        return 0;
    }

    static int RunGranetInflectorApplet(int argc, const char** argv) {
        TLangMask langs;
        TString text;
        TString grams;
        bool isVerbose = false;
        ReadArguments(argc, argv, &text, &langs, &grams, &isVerbose);

        IInputStream* in = &Cin;
        TStringInput textInput(text);
        if (!text.empty()) {
            in = &textInput;
        }

        TUtf16String line;
        while (in->ReadLine(line)) {
            DumpGranetInflector(line, langs, grams, isVerbose, &Cout);
        }
        return 0;
    }

    int Run(int argc, const char** argv) {
        TModChooser modChooser;

        modChooser.AddMode("normalizer", RunNormalizerApplet, "Dump some normalizations");
        modChooser.AddMode("lemmer", RunLemmerApplet, "Dump NLemmer::AnalyzeWord");
        modChooser.AddMode("tokenizer", RunTokenizerApplet, "Dump TNlpTokenizer");
        modChooser.AddMode("granet-inflector", RunGranetInflectorApplet, "Dump NGranet::InflectPhrase");

        return modChooser.Run(argc, argv);
    }

} // namespace

int main(int argc, const char** argv) {
    try {
        return Run(argc, argv);
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
        return 1;
    }
}
