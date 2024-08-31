#pragma once

#include <alice/bass/libs/push_notification/handlers/handler.h>
#include <alice/bass/libs/push_notification/scheme/scheme.sc.h>

#include <alice/bass/util/error.h>
#include <library/cpp/scheme/scheme.h>

namespace NBASS::NPushNotification {

using TQuasarPlayVideoAction = NBassPushNotification::TQuasarPlayVideoAction<TSchemeTraits>;
using TQuasarPlayVideoActionSchemeHolder = TSchemeHolder<TQuasarPlayVideoAction>;

namespace NQuasarVideoPush {

constexpr TStringBuf QUASAR_PLAY_VIDEO_BY_DESCRIPTOR = "quasar.play_video_by_descriptor";

NSc::TValue CreateVideoDescriptor(TStringBuf playUri, TStringBuf provider, TStringBuf providerItemId);

TString GenerateActionPayload(TStringBuf actionName, const NSc::TValue &videoDescriptor);

}  // namespace NQuasarVideoPush

}  // namespace NBASS::NPushNotification
