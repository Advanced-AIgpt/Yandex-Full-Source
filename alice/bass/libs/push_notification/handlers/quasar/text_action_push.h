#pragma once

#include <alice/bass/libs/push_notification/handlers/handler.h>
#include <alice/bass/libs/push_notification/scheme/scheme.sc.h>

#include <alice/bass/util/error.h>
#include <library/cpp/scheme/scheme.h>

namespace NBASS::NPushNotification {

using TQuasarTextAction = NBassPushNotification::TQuasarTextAction<TSchemeTraits>;
using TQuasarTextActionSchemeHolder = TSchemeHolder<TQuasarTextAction>;

namespace NQuasarTextActionPush {

NSc::TValue GenerateActionPayload(const NSc::TValue& textActions);

TString GenerateTextActionPayload(const NSc::TValue& textActions);

}  // namespace NQuasarTextActionPush

}  // namespace NBASS::NPushNotification
