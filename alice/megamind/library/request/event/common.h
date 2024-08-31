#pragma once

#include <alice/megamind/protos/scenarios/request.pb.h>
#include <util/generic/maybe.h>
#include <util/generic/string.h>


namespace NAlice {
namespace NMegamind {

inline
void OnTextEvent(NScenarios::TInput* input, const TString& originalUtterance,
                 const TMaybe<TString>& normalizedUtterance, bool fromSuggest) {
    auto* text = input->MutableText();
    text->SetRawUtterance(originalUtterance);
    if (normalizedUtterance.Defined()) {
        text->SetUtterance(normalizedUtterance.GetRef());
    }
    input->MutableText()->SetFromSuggest(fromSuggest);
}

} // namespace NMegamind
} // namespace NAlice
