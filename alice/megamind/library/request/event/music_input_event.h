#pragma once

#include <alice/megamind/library/request/event/event.h>

namespace NAlice {

class TMusicInputEvent: public IEvent {
public:
    using IEvent::IEvent;

    bool IsMusicInput() const override {
        return true;
    }

    void FillScenarioInput(const TMaybe<TString>& /* normalizedUtterance */, NScenarios::TInput* input) const override {
        *input->MutableMusic()->MutableMusicResult() = SpeechKitEvent().GetMusicResult();
    }
};

} // namespace NAlice
