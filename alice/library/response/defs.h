#pragma once

#include <util/generic/string.h>

namespace NAlice::NResponse {

inline const TString TYPE_ANALYTICS = "analytics_info";
inline const TString TYPE_ATTENTION = "attention";
inline const TString TYPE_COMMAND = "command";
inline const TString TYPE_ERROR = "error";
inline const TString TYPE_STOP_LISTENING = "stop_listening";
inline const TString TYPE_SILENT_RESPONSE = "silent_response";

inline const TString CLIENT_ACTION = "client_action";
inline const TString SERVER_ACTION = "server_action";
inline const TString UNIPROXY_ACTION = "uniproxy_action";

inline const TString DIV_CARD = "div_card";
inline const TString SIMPLE = "simple";
inline const TString SIMPLE_TEXT = "simple_text";

inline const TStringBuf LISTENING_IS_POSSIBLE = "listening_is_possible";

inline const TString THREE_DOTS = "...";

}
