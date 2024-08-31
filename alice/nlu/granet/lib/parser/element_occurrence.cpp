#include "element_occurrence.h"
#include <util/string/printf.h>

namespace NGranet {

void TElementOccurrence::DumpTree(const TGrammar::TConstRef& grammar, const TSample::TConstRef& sample,
    IOutputStream* log, const TString& indent) const
{
    Y_ENSURE(log);
    *log << indent << "( " << sample->GetJoinedTokens() << " ) LogProb  Interval  Element" << Endl;
    DumpRecursive(grammar, sample, log, indent, "");
}

void TElementOccurrence::ToNonterminalMarkup(const TGrammar::TConstRef& grammar, const TSample::TConstRef& sample,
    TVector<TSlotMarkup>& markupSlots) const
{
    // We need this method only for checking filler tag, so it's enough to check onyl leaf nodes
    if (!Children.empty()) {
        for (const TElementOccurrence::TRef& child : Children) {
            child->ToNonterminalMarkup(grammar, sample, markupSlots);
        }
    } else {
        TSlotMarkup& markupSlot = markupSlots.emplace_back();
        markupSlot.Interval = sample->ConvertPositionToText(Interval);
        markupSlot.Name = grammar->GetData().Elements[ElementId].Name;
    }
}

void TElementOccurrence::DumpRecursive(const TGrammar::TConstRef& grammar, const TSample::TConstRef& sample,
    IOutputStream* log, const TString& indent1, const TString& indent2) const
{
    Y_ENSURE(log);
    *log << indent1 << "( " << sample->PrintMaskedTokens(Interval) << " ) "
        << Sprintf("%7.2f  ", LogProb)
        << RightPad(Interval, 9) << " "
        << indent2 << grammar->GetData().Elements[ElementId].Name << Endl;
    for (const TElementOccurrence::TRef& child : Children) {
        child->DumpRecursive(grammar, sample, log, indent1, indent2 + "  ");
    }
}

} // namespace NGranet
