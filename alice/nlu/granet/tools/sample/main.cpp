#include <alice/nlu/granet/lib/granet.h>
#include <library/cpp/getopt/last_getopt.h>
#include <util/generic/ptr.h>

using namespace NGranet;

void ReadOptions(int argc, const char *argv[], TFsPath* grammarPath) {
    Y_ENSURE(grammarPath);

    NLastGetopt::TOpts opts = NLastGetopt::TOpts::Default();

    opts.AddLongOption("grammar")
        .AddShortName('g')
        .StoreResult(grammarPath)
        .Required()
        .Help("Path to the config file (like this: alice/nlu/data/ru/granet/main.grnt)");

    opts.SetFreeArgsNum(0);
    opts.AddHelpOption('h');
    NLastGetopt::TOptsParseResult args(&opts, argc, argv);
}

int main(int argc, const char *argv[]) {
    TFsPath grammarPath;
    ReadOptions(argc, argv, &grammarPath);

    TGrammar::TRef grammar = CompileGrammarFromPath(grammarPath, {});
    grammar->Dump(&Cout);

    while (true) {
        TString line;
        if (!Cin.ReadLine(line)) {
            break;
        }
        TSample::TRef sample = CreateSample(line, grammar->GetLanguage());
        FetchEntities(sample, {});
        sample->Dump(&Cout);

        TVector<TParserFormResult::TConstRef> forms = ParseSample(grammar, sample);

        bool hasMatches = false;
        for (const TParserFormResult::TConstRef& form : forms) {
            if (!form->IsPositive()) {
                continue;
            }
            hasMatches = true;
            Cout << form->GetName() << ": " << form->ToMarkup().PrintMarkup(0) << Endl;
            form->Dump(&Cout, "  ");
        }
        if (!hasMatches) {
            Cout << "No matches" << Endl;
            continue;
        }
    }
    return 0;
}
