#include "debug.h"
#include "state_dumper.h"
#include <alice/nlu/granet/lib/utils/string_utils.h>
#include <alice/nlu/granet/lib/parser/parser.h>
#include <library/cpp/iterator/zip.h>
#include <util/charset/wide.h>
#include <util/generic/adaptor.h>
#include <util/generic/utility.h>
#include <util/string/join.h>
#include <util/string/printf.h>

namespace NGranet {

// ~~~~ TParserDebugInfo ~~~~

TParserDebugInfo::TParserDebugInfo(const TGrammar::TConstRef& grammar, const TSample::TConstRef& sample,
        TVector<TParserStateList>&& mainChart, TVector<TParserStateList>&& fillerChart)
    : Grammar(grammar)
    , Sample(sample)
    , MainChart(std::move(mainChart))
    , FillerChart(std::move(fillerChart))
{
    Y_ENSURE(Grammar);
    Y_ENSURE(Sample);
}

TString TParserDebugInfo::FindParserBlockerTokenStr() const {
    const size_t tokenIndex = FindParserBlockerTokenIndex();
    if (tokenIndex >= Sample->GetTokens().size()) {
        return "";
    }
    return Sample->GetTokens()[tokenIndex];
}

size_t TParserDebugInfo::FindParserBlockerTokenIndex() const {
    for (size_t i = 1; i <= Sample->GetTokens().size(); i++) {
        if (MainChart[i].GetStateCount() == 0 && FillerChart[i].GetStateCount() == 0) {
            return i - 1;
        }
    }
    return Sample->GetTokens().size();
}

TVector<const TParserState*> TParserDebugInfo::GetOccurrences() const {
    TVector<const TParserState*> occurrences;
    CollectOccurrences(MainChart, &occurrences);
    CollectOccurrences(FillerChart, &occurrences);
    return occurrences;
}

void TParserDebugInfo::Dump(IOutputStream* log, const TString& indent) const {
    Y_ENSURE(log);

    DumpNumberOfStates(log, indent);
    DumpOccurrences(log, indent);
}

void TParserDebugInfo::DumpNumberOfStates(IOutputStream* log, const TString& indent) const {
    *log << indent << "Number of states (root+filler):" << Endl;
    *log << indent << "  ";
    for (size_t i = 0; i <= Sample->GetTokens().size(); i++) {
        if (i > 0) {
            *log << ' ' << Sample->GetTokens()[i - 1] << ' ';
        }
        *log << '(' << PrintStateCount(MainChart[i]) << '+' << PrintStateCount(FillerChart[i]) << ')';
    }
    *log << Endl;
}

// static
TString TParserDebugInfo::PrintStateCount(const TParserStateList& list) {
    return TStringBuilder() << list.GetStateCount() << (list.GetStateLimitHasBeenReached() ? "(!)" : "");
}

void TParserDebugInfo::DumpOccurrences(IOutputStream* log, const TString& indent) const {
    TVector<const TParserState*> occurrences;
    CollectOccurrences(MainChart, &occurrences);
    CollectOccurrences(FillerChart, &occurrences);

    *log << indent << "Occurrences:" << Endl;
    *log << indent << "  ( " << Sample->GetJoinedTokens() << " ) LogProb  Element" << Endl;

    for (const TParserState* state : occurrences) {
        *log << indent << "  ( " << Sample->PrintMaskedTokens(state->Interval.ToInterval<size_t>()) << " ) ";
        *log << Sprintf("%7.2f  ", state->LogProb);
        *log << state->Element->Name;
        if (state->IsNegative()) {
            *log << "  " << (state->IsForced() ? "%force_negative" : "%negative");
        } else if (state->IsForced()) {
            *log << "  %force_positive";
        }
        *log << Endl;
    }
}

// static
void TParserDebugInfo::CollectOccurrences(const TVector<TParserStateList>& chart, TVector<const TParserState*>* result) {
    Y_ENSURE(result);

    // Tuple {-level, element_name, interval, -logprob, state}
    TVector<std::tuple<int, TStringBuf, NNlu::TIntInterval, float, const TParserState*>> occurrences;
    for (const TParserStateList& list : chart) {
        for (int level = 0; level < list.GetLevels().ysize(); level++) {
            const TDeque<TParserState>& states = list.GetLevels()[level];
            for (const TParserState& state : states) {
                if (!state.IsComplete() || state.IsDisabled) {
                    continue;
                }
                occurrences.emplace_back(-level, state.Element->Name, state.Interval, -state.LogProb, &state);
            }
        }
    }
    Sort(occurrences);
    for (const auto& [level, name, interval, logProb, state] : occurrences) {
        result->push_back(state);
    }
}

} // namespace NGranet
