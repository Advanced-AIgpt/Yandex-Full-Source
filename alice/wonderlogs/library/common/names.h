#pragma once

#include <util/generic/strbuf.h>

namespace NAlice::NWonderlogs {

inline constexpr TStringBuf MESSAGE_ID = "message_id";
inline constexpr TStringBuf REQUEST_ID = "request_id";
inline constexpr TStringBuf RESPONSE_ID = "response_id";
inline constexpr TStringBuf CONNECT_SESSION_ID = "session_id";
inline constexpr TStringBuf SETRACE_URL_PREFIX = "https://setrace.yandex-team.ru/ui/alice/sessionsList?trace_by=";
inline constexpr TStringBuf UUID = "uuid";

} // namespace NAlice::NWonderlogs
