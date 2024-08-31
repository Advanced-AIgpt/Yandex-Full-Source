#pragma once

#include <alice/bass/libs/push_notification/handlers/handler.h>
#include <alice/bass/libs/push_notification/scheme/scheme.sc.h>

#include <library/cpp/scheme/scheme.h>

namespace NBASS::NPushNotification {

using TQuasarRepeatPhraseAction = NBassPushNotification::TQuasarRepeatPhraseAction<TSchemeTraits>;
using TQuasarRepeatPhraseActionSchemeHolder = TSchemeHolder<TQuasarRepeatPhraseAction>;

namespace NQuasarRepeatPhraseActionPush {

inline constexpr TStringBuf QUASAR_IOT_REPEAT_PHRASE = "personal_assistant.scenarios.quasar.iot.repeat_phrase";

TString GenerateRepeatPhraseActionPayload(const NSc::TValue& phraseActions);

}  // namespace NQuasarRepeatPhraseActionPush

}  // namespace NBASS::NPushNotification
