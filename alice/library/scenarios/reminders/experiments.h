#pragma once

#include <util/generic/strbuf.h>

namespace NAlice::NRemindersApi {

// Create for temporary solution to disable redirect push.
inline constexpr TStringBuf EXPFLAG_REMINDERS_DISABLE_REDIRECT_PUSH = "reminders_disable_redirect_push";

} // NAlice::NRemindersApi
