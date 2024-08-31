#include "frame.h"

namespace NAlice::NHollywood {

TMaybe<TFrame> TryGetNewsFrame(const TScenarioRunRequestWrapper& runRequest) {
    const auto& input = runRequest.Input();
    return TryGetFrame(GET_NEWS_FRAME, GetCallbackFrame(input.GetCallback()), input);
}

template<typename T>
TMaybe<T> TryGetSlotValue(const TMaybe<TFrame>& frame, const TStringBuf slotName) {
    if (frame) {
        const auto slot = frame->FindSlot(slotName);
        if (slot.IsValid()) {
            return slot->Value.As<T>();
        }
    }
    return {};
}

bool GetHasIntroAndEnding(const TMaybe<TFrame>& frame, const TScenarioRunRequestWrapper& runRequest) {
    return !TryGetSlotValue<bool>(frame, SKIP_INTRO_AND_ENDING_SLOT).GetOrElse(runRequest.HasExpFlag("alice_news_skip_intro_and_ending"));
}

bool GetHasIntroAndEnding(const TScenarioRunRequestWrapper& runRequest) {
    return GetHasIntroAndEnding(TryGetNewsFrame(runRequest), runRequest);
}

bool GetDisableVoiceButtons(const TMaybe<TFrame>& frame) {
    return TryGetSlotValue<bool>(frame, DISABLE_VOICE_BUTTONS_SLOT).GetOrElse(false);
}

bool GetDisableVoiceButtons(const TScenarioRunRequestWrapper& runRequest) {
    return GetDisableVoiceButtons(TryGetNewsFrame(runRequest));
}

TMaybe<int> TryGetNewsArrayPosition(const TMaybe<TFrame>& frame) {
    return TryGetSlotValue<int>(frame, NEWS_IDX_SLOT);
}

bool IsSingleNewsRequest(const TMaybe<TFrame>& frame) {
    return TryGetSlotValue<bool>(frame, SINGLE_NEWS_SLOT).GetOrElse(false);
}

} // namespace NAlice::NHollywood
