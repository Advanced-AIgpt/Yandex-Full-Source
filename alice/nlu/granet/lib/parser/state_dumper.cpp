#include "state_dumper.h"
#include <alice/nlu/granet/lib/utils/utils.h>
#include <alice/nlu/granet/lib/utils/string_utils.h>
#include <alice/nlu/granet/lib/parser/parser.h>
#include <util/charset/wide.h>
#include <util/generic/adaptor.h>
#include <util/string/join.h>
#include <util/string/printf.h>

namespace NGranet {

// ~~~~ TParserStateDumper ~~~~

void TParserStateDumper::DumpStateList(bool isFiller, int pos, const TParserStateList& list,
    IOutputStream* log, const TString& indent)
{
    Y_ENSURE(log);
    *log << indent << "State list " << pos << (isFiller ? " (filler):" : ":") << Endl;
    DumpElements(list, log, indent + "  ");
    DumpStates(list, log, indent + "  ");
}

void TParserStateDumper::DumpElements(const TParserStateList& list, IOutputStream* log, const TString& indent) {
    Y_ENSURE(log);
    TMap<TElementId, const TGrammarElement*> elements;
    for (const TDeque<TParserState>& level : list.GetLevels()) {
        for (const TParserState& state : level) {
            elements[state.Element->Id] = state.Element;
        }
    }
    *log << indent << "Referenced elements:" << Endl;
    for (const auto& [id, element] : elements) {
        *log << indent << "  " << PrintElement(*element) << Endl;
    }
}

void TParserStateDumper::DumpStates(const TParserStateList& list, IOutputStream* log, const TString& indent) {
    Y_ENSURE(log);
    *log << indent << "State#  Key#   It#    SlnLP   OwnLP       key|state  B  E  L  Element                    Event                 Comment" << Endl;
    for (const TDeque<TParserState>& level : list.GetLevels()) {
        for (const TParserState& state : level) {
            DumpState(state, log, indent);
        }
    }
}

static TString PrintStateId(const TParserState& state) {
    return Sprintf("S%02d%03d", state.Interval.End, state.Index);
}

void TParserStateDumper::DumpState(const TParserState& state, IOutputStream* log, const TString& indent) {
    Y_ENSURE(log);
    *log << indent;
    *log << PrintStateId(state);
    *log << " K" << LeftPad(StateKeyIds.EnsureId(state), 4, '0');
    *log << " T" << LeftPad(TrieIteratorIds.EnsureId(state.TrieIterator), 3, '0') << 'D' << state.TrieIteratorDepth;
    *log << Sprintf(" %7.2f", state.SolutionLogProbUpperBound);
    *log << Sprintf(" %7.2f", state.LogProb);
    *log << " [";
    *log << (state.ConstrainedIterationCount > 1 ? "I" + ToString(state.ConstrainedIterationCount) : "  ");
    *log << (state.SetOfCompleteRulesOfBag != 0 ? "R" + ToString(CountBits(state.SetOfCompleteRulesOfBag)) : "  ");
    *log << (!state.HasWordsInCurrentIteration() ? 'E' : ' ');
    *log << (state.IsPassedFiller() ? 'f' : ' ');
    *log << (state.IsWaitingForChild() ? 'W' : ' ');
    *log << (state.IsComplete() ? 'C' : ' ');
    *log << '|';
    *log << (state.IsDisabled ? 'X' : ' ');
    *log << (state.BetterAlternative ? '-' : (state.WorseAlternative ? '+' : ' '));
    *log << (state.IsForced() ? 'F' : ' ');
    *log << (state.IsNegative() ? 'N' : ' ');
    *log << "]";
    *log << ' ' << LeftPad(state.Interval.Begin, 2);
    *log << ' ' << LeftPad(state.Interval.End, 2);
    *log << ' ' << LeftPad(state.Element->Level, 2);
    *log << "  E" << RightPad(state.Element->Id, 5);
    *log << " " << FitToWidth(state.Element->Name, 20);
    *log << "  " << RightPad(state.EventType, 20);

    if (state.IsWaitingForChild()) {
        Y_ASSERT(state.PredictedChild);
        *log << "  wait " << FitToWidth(PrintElement(*state.PredictedChild->Element), 40);
    } else if (state.WaitingParentsList != nullptr) {
        TVector<TString> ids;
        for (const TParserState* parent = state.WaitingParentsList; parent != nullptr; parent = parent->WaitingParentsList ) {
            ids.push_back(PrintStateId(*parent));
        }
        *log << "  parents: " << JoinSeq(", ", ids);
    }
    if (state.PassedChild) {
        *log << "  pass " << FitToWidth(PrintElement(*state.PassedChild->Element), 40);
    }
    if (state.IsComplete()) {
        *log << "  ";
        if (state.Element->IsEntity()) {
            *log << "entity";
        } else {
            *log << "rule";
        }
        *log << " " << state.CompleteRules.RuleIndex;
        if (state.CompleteRules.RuleCount > 1) {
            *log << "-" << state.CompleteRules.RuleIndex + state.CompleteRules.RuleCount - 1;
        }
    }
    *log << Endl;
}

// static
TString TParserStateDumper::PrintElement(const TGrammarElement& element) {
    return TStringBuilder() << "E" << element.Id << " = " << element.Name;
}

} // namespace NGranet
