#include "event.h"

namespace NAlice::NMegamind {

bool TEventComponent::TView::HasEvent() const {
    const auto& event = Event_.Event();
    return event.IsInitialized() && event.HasType();
}

TMaybe<bool> TEventComponent::TView::IsEOU() const {
    const auto& event = Event();
    if (!HasEvent()) {
        return Nothing();
    }
    if (event.GetType() != EEventType::voice_input) {
        return true;
    }
    if (!event.HasEndOfUtterance()) {
        return Nothing();
    }
    return event.GetEndOfUtterance();
}

} // namespace NAlice::NMegamind
