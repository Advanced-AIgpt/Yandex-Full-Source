#pragma once

#include <alice/bass/libs/push_notification/handlers/handler.h>
#include <alice/bass/libs/push_notification/scheme/scheme.sc.h>

#include <library/cpp/scheme/scheme.h>

namespace NBASS::NPushNotification {

using TQuasarMMSemanticFrameAction = NBassPushNotification::TQuasarMMSemanticFrameAction<TSchemeTraits>;
using TQuasarMMSemanticFrameActionSchemeHolder = TSchemeHolder<TQuasarMMSemanticFrameAction>;

namespace NQuasarMMSemanticFrameActionPush {

TString GenerateMMSemanticFrameActionPayload(const NSc::TValue& mmSemanticFramePayload);

}  // namespace NQuasarMMSemanticFrameActionPush

}  // namespace NBASS::NPushNotification
