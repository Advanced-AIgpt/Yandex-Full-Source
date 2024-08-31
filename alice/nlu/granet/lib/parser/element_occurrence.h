#pragma once

#include <alice/nlu/granet/lib/grammar/grammar.h>
#include <alice/nlu/granet/lib/sample/markup.h>
#include <alice/nlu/granet/lib/sample/sample.h>
#include <alice/nlu/granet/lib/sample/tag.h>
#include <alice/nlu/granet/lib/utils/utils.h>
#include <alice/nlu/libs/interval/interval.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/stream/output.h>

namespace NGranet {

class TElementOccurrence : public TThrRefBase {
public:
    using TRef = TIntrusivePtr<TElementOccurrence>;
    using TConstRef = TIntrusiveConstPtr<TElementOccurrence>;

public:
    // Position of element in array of tokens.
    NNlu::TInterval Interval;

    // Matched element.
    TElementId ElementId = UNDEFINED_ELEMENT_ID;

    float LogProb = 0;

    TVector<TElementOccurrence::TRef> Children;

public:
    void PushChildFront(const TElementOccurrence::TRef& child) {
        Children.insert(Children.begin(), child);
    }

    void DumpTree(const TGrammar::TConstRef& grammar, const TSample::TConstRef& sample,
        IOutputStream* log, const TString& indent = "") const;

    void ToNonterminalMarkup(const TGrammar::TConstRef& grammar, const TSample::TConstRef& sample,
        TVector<TSlotMarkup>& markupSlots) const;

private:
    void DumpRecursive(const TGrammar::TConstRef& grammar, const TSample::TConstRef& sample,
        IOutputStream* log, const TString& indent1, const TString& indent2) const;
};

} // namespace NGranet
