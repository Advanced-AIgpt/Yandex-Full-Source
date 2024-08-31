#pragma once

#include <util/generic/strbuf.h>

namespace NAlice::NSearch {

inline constexpr TStringBuf LSTM_NAME = "personal_assistant.scenarios.other";
inline constexpr TStringBuf SEARCH_FORM = "personal_assistant.scenarios.search";

// Search like intents
inline constexpr TStringBuf FIND_POI_FORM = "personal_assistant.scenarios.find_poi";
inline constexpr TStringBuf OPEN_SITE_OR_APP_FORM = "personal_assistant.scenarios.open_site_or_app";
inline constexpr TStringBuf HOW_MUCH_FORM = "personal_assistant.scenarios.how_much";
inline constexpr TStringBuf TRANSLATE_FORM = "personal_assistant.scenarios.translate";
inline constexpr TStringBuf TV_BROADCAST_FORM = "personal_assistant.scenarios.tv_broadcast";
inline constexpr TStringBuf CONVERT_FORM = "personal_assistant.scenarios.convert";

inline constexpr TStringBuf SLOT_QUERY = "query";

} // namespace NAlice::NSearch
