#pragma once

#include <alice/megamind/library/request/event/event.h>
#include <alice/megamind/library/request/event/common.h>
#include <alice/megamind/protos/common/events.pb.h>

namespace NAlice {

class TSuggestedInputEvent: public IEvent {
public:
    explicit TSuggestedInputEvent(const TEvent& event)
        : IEvent(event)
        , Utterance_(event.GetText())
    {
    }

    const TString& GetUtterance() const override {
        return Utterance_;
    }

    bool HasUtterance() const override {
        return true;
    }

    void FillScenarioInput(const TMaybe<TString>& normalizedUtterance, NScenarios::TInput* input) const override {
        NMegamind::OnTextEvent(input, GetUtterance(), normalizedUtterance, /* fromSuggest= */ true);
    }

private:
    TString Utterance_;
};

} // namespace NAlice
