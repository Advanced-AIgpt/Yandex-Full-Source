#pragma once

#include <util/generic/hash.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>

namespace NAlice::NFindPoi {

inline constexpr TStringBuf SLOT_OPEN = "open";
inline constexpr TStringBuf SLOT_WHAT = "what";
inline constexpr TStringBuf SLOT_WHERE = "where";

inline constexpr TStringBuf SLOT_NAMED_LOCATION_TYPE = "named_location";
inline constexpr TStringBuf SLOT_POI_OPEN_TYPE = "poi_open";
inline constexpr TStringBuf SLOT_SPECIAL_LOCATION_TYPE = "special_location";

inline constexpr TStringBuf FIND_POI = "personal_assistant.scenarios.find_poi";

} // namespace NAlice::NFindPoi
