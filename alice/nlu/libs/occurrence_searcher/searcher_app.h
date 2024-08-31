#pragma once
#include "occurrence_searcher.h"
#include <library/cpp/getopt/last_getopt.h>
#include <util/generic/string.h>
#include <util/stream/input.h>


template<typename TValue>
class TSearcherApp {
public:
    int Run(int argc, const char* argv[]) {
        ParseArgs(argc, argv);

        NAlice::NNlu::TOccurrenceSearcher<TValue> searcher(TBlob::FromFile(OccurrenceSearcherDataPath));
        for (TString line; Cin.ReadLine(line); ) {
            for (const auto& occurrence : searcher.Search(line)) {
                PrintOccurrence(occurrence, line);
            }
            Cout << '\n';
        }
        return 0;
    }

    ~TSearcherApp() = default;

protected:
    virtual void PrintOccurrence(const NAlice::NNlu::TOccurrence<TValue>& occurrence, const TString& text) const {
        const auto occurrenceString = text.substr(occurrence.Begin, occurrence.End - occurrence.Begin);
        Cout << occurrenceString << ' ' << occurrence.Value << '\n';
    }

private:
    void ParseArgs(int argc, const char* argv[]) {
        auto opts = NLastGetopt::TOpts::Default();
        opts.AddHelpOption('h');
        opts.SetFreeArgsNum(1);
        opts.SetFreeArgTitle(0, "OCCURRENCE_SEARCHER_DATA_PATH", "Path to the occurrence searcher data (built by rule's `build_automaton` tool)");
        NLastGetopt::TOptsParseResult optsParseResult(&opts, argc, argv);
        OccurrenceSearcherDataPath = optsParseResult.GetFreeArgs().front();
    }

private:
    TString OccurrenceSearcherDataPath;
};

