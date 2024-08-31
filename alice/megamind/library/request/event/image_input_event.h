#pragma once

#include <alice/megamind/library/request/event/event.h>

namespace NAlice {

class TImageInputEvent: public IEvent {
public:
    using IEvent::IEvent;

    bool IsImageInput() const override {
        return true;
    }

    void FillScenarioInput(const TMaybe<TString>& normalizedUtterance, NScenarios::TInput* input) const override;
};

} // namespace NAlice
