#include "dataset_statistics.h"
#include <alice/nlu/granet/lib/compiler/compiler.h>
#include <alice/nlu/granet/lib/granet.h>
#include <alice/nlu/granet/lib/parser/multi_parser.h>
#include <alice/nlu/granet/lib/utils/string_utils.h>
#include <alice/nlu/granet/lib/utils/utils.h>
#include <util/charset/utf8.h>
#include <util/generic/adaptor.h>
#include <util/generic/is_in.h>
#include <util/string/join.h>
#include <util/string/printf.h>

namespace NGranet {

// ~~~~ TSlowestSamplesCollector ~~~~

void TSlowestSamplesCollector::Accumulate(const TString& text, double timeUs) {
    SlowestSamples.emplace(-timeUs, text);
    while (SlowestSamples.size() > 20) {
        SlowestSamples.pop();
    }
}

void TSlowestSamplesCollector::Write(const TFsPath& dir) const {
    dir.MkDirs();
    TFileOutput out(dir / "slowest.tsv");
    out << "time_us\ttext\n";
    for (const auto& [timeUs, text] : Sorted(SlowestSamples.Container())) {
        out << Sprintf("%5.0f", -timeUs) << '\t' << text << '\n';
    }
}

// ~~~~ TElementDictionaryCollector ~~~~

void TElementDictionaryCollector::Accumulate(const TSampleMarkup& markup) {
    if (!markup.IsPositive) {
        return;
    }
    for (const TSlotMarkup& slot : markup.Slots) {
        const TString text = markup.Text.substr(slot.Interval.Begin, slot.Interval.Length());
        TTagInfo& info = Dictionaries[slot.Name][text];
        info.Count++;
        info.Examples.insert(markup.PrintMarkup(0));
    }
}

void TElementDictionaryCollector::Write(const TFsPath& dir) const {
    WriteBrief(dir / "slot_mining_brief");
    WriteVerbose(dir / "slot_mining_verbose");
}

void TElementDictionaryCollector::WriteBrief(const TFsPath& dir) const {
    dir.MkDirs();
    for (const auto& [name, dictionary] : Dictionaries) {
        TFileOutput out(dir / (name + ".txt"));
        for (const TString& text : OrderedSetOfKeys(dictionary)) {
            out << text << Endl;
        }
    }
}

void TElementDictionaryCollector::WriteVerbose(const TFsPath& dir) const {
    dir.MkDirs();
    for (const auto& [name, dictionary] : Dictionaries) {
        // Sort dictionary items by weight.
        auto items = ToVector<std::pair<TString, TTagInfo>>(dictionary);
        SortBy(items, [](const auto& item) {return std::make_pair(-item.second.Count, item.first);});

        TFileOutput out(dir / (name + ".txt"));
        for (const auto& [text, info] : items) {
            out << LeftJustify(text, 25);
            out << " # " << LeftPad(info.Count, 4);
            out << " (" << JoinSeq(" | ", info.Examples) << ")" << Endl;
        }
    }
}

// ~~~~ TNegativeNgramsCollector ~~~~

TNegativeNgramsCollector::TNegativeNgramsCollector() {
    Ngrams.resize(N);
}

void TNegativeNgramsCollector::Accumulate(const TString& text) {
    TVector<TString> words = StringSplitter(text).Split(' ');

    for (size_t n = 0; n < N; n++) {
        for (size_t i = 0; i + n < words.size(); i++) {
            const TString ngram = JoinRange(" ", words.begin() + i, words.begin() + i + n + 1);
            Ngrams[n][ngram]++;
        }
    }
}

void TNegativeNgramsCollector::Write(const TFsPath& dir) const {
    const TFsPath ngramsDir = dir / "n_grams";
    ngramsDir.MkDirs();

    for (size_t n = 0; n < N; n++) {
        TFileOutput out(ngramsDir / (ToString(n + 1) + "_gram.tsv"));
        out << "ngram\tcount\n";

        TVector<std::pair<TString, size_t>> sortedNgrams(Ngrams[n].begin(), Ngrams[n].end());
        SortBy(sortedNgrams, [](const auto& item) {return -item.second;});

        for (const auto& [ngram, count] : sortedNgrams) {
            out << ngram << '\t' << count << '\n';
        }
    }
}

// ~~~~ TParserBlockerCollector ~~~~

namespace {

    const size_t MAX_BLOCKERS_COUNT = 20;
    const TString GRANET_TESTING_SUGGEST_ENTITY = "custom.granet_testing_suggest.ifexp.bg_enable_granet_testing_suggest";

    constexpr std::array<TStringBuf, 3> NON_BLOCKERS = {
        "INTERNAL_ERROR",
        "STATE_LIMIT",
        "WEAK_TEXT",
    };

    TVector<TString> FindNonterminals(const TGrammar::TConstRef& grammar, TStringBuf blocker) {
        if (IsIn(NON_BLOCKERS, blocker)) {
            return {};
        }

        TSample::TRef sample = CreateSample(blocker, grammar->GetLanguage());
        TParserEntityResult::TRef entity = TMultiParser::Create(grammar, sample, true)->ParseEntity(GRANET_TESTING_SUGGEST_ENTITY);
        if (!entity->IsPositive()) {
            return {};
        }
        TVector<TString> nonterminals;
        for (const TParserState* state : entity->GetDebugInfo()->GetOccurrences()) {
            const auto& nonterminal = state->Element->Name;
            if (nonterminal.StartsWith("$Common")) {
                nonterminals.push_back(nonterminal);
            }
        }
        return nonterminals;
    }

} // namespace

void TParserBlockerCollector::Accumulate(const TSampleProcessorResult& result) {
    if (result.Blocker.empty()) {
        return;
    }
    BlockerToSamples[result.Blocker].push_back(result.Result.Text);
}

void TParserBlockerCollector::Write(const TFsPath& dir) const {
    dir.MkDirs();

    auto rows = GetSortedBlockers();

    TFileOutput outAggregated(dir / "blockers_aggregated.tsv");
    outAggregated << "count\tblocker\tsample\n";
    for (const auto& [blocker, samples] : rows) {
        outAggregated << samples.size() << '\t' << blocker << '\t' << samples[0] << '\n';
    }

    TFileOutput outAll(dir / "blockers_all.tsv");
    outAll << "blocker\tsample\n";
    for (const auto& [blocker, samples] : rows) {
        for (const auto& sample : samples) {
            outAll << blocker << '\t' << sample << '\n';
        }
    }
}

bool TParserBlockerCollector::HasBlockers() const {
    return !BlockerToSamples.empty();
}

void TParserBlockerCollector::PrintSuggests(const TGrammar::TConstRef& grammar, IOutputStream* out, const TString& indent) const {
    Y_ENSURE(out);

    auto blockers = GetSortedBlockers();

    const bool isTruncated = blockers.size() > MAX_BLOCKERS_COUNT;
    if (isTruncated) {
        blockers.erase(blockers.begin() + MAX_BLOCKERS_COUNT, blockers.end());
    }

    const size_t countColumnWidth = 7;
    const size_t sampleColumnWidth = 30;
    size_t blockerColumnWidth = 10;
    for (const auto& [blocker, samples] : blockers) {
        blockerColumnWidth = Max(blockerColumnWidth, GetNumberOfUTF8Chars(blocker));
    }

    *out << indent;
    *out << RightJustify("Count", countColumnWidth) << "  ";
    *out << LeftJustify("Blocker", blockerColumnWidth) << "  ";
    *out << LeftJustify("Sample", sampleColumnWidth) << "  ";
    *out << "Nonterminals" << Endl;
    for (const auto& [blocker, samples] : blockers) {
        *out << indent;
        *out << RightJustify(ToString(samples.size()), countColumnWidth) << "  ";
        *out << LeftJustify(blocker, blockerColumnWidth) << "  ";
        *out << FitToWidth(samples[0], sampleColumnWidth, "...", ' ') << "  ";
        *out << JoinSeq(" ", FindNonterminals(grammar, blocker)) << Endl;
    }
    if (isTruncated) {
        *out << indent << RightJustify("...", countColumnWidth) << Endl;
    }
}

TVector<std::pair<TString, TVector<TString>>> TParserBlockerCollector::GetSortedBlockers() const {
    auto rows = ToVector<std::pair<TString, TVector<TString>>>(BlockerToSamples);
    SortBy(rows, [](const auto& row) {return std::make_pair(-row.second.ysize(), row.first);});
    return rows;
}

} // namespace NGranet
