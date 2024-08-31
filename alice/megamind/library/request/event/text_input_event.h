#pragma once

#include <alice/megamind/library/request/event/event.h>
#include <alice/megamind/library/request/event/common.h>
#include <alice/megamind/protos/common/events.pb.h>

namespace NAlice {
namespace {

TEvent MakeTextInputEvent(const TString& utterance) {
    TEvent event{};
    event.SetType(EEventType::text_input);
    event.SetText(utterance);
    return event;
}

} // namespace

class TTextInputEvent: public IEvent {
public:
    explicit TTextInputEvent(const TEvent& event, bool isUserGenerated = true)
        : IEvent(event)
        , Utterance_(event.GetText())
        , IsUserGenerated_(isUserGenerated)
    {
    }

    TTextInputEvent(const TString& utterance, bool isUserGenerated = true)
        : TTextInputEvent(MakeTextInputEvent(utterance), isUserGenerated)
    {
    }

    const TString& GetUtterance() const override {
        return Utterance_;
    }

    bool HasUtterance() const override {
        return true;
    }

    bool IsTextInput() const override {
        return true;
    }

    bool IsUserGenerated() const override {
        return IsUserGenerated_;
    }

    void FillScenarioInput(const TMaybe<TString>& normalizedUtterance, NScenarios::TInput* input) const override {
        NMegamind::OnTextEvent(input, GetUtterance(), normalizedUtterance, /* fromSuggest= */ false);
    }

private:
    TString Utterance_;
    bool IsUserGenerated_;
};

} // namespace NAlice
