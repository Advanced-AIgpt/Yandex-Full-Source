#pragma once

#include <util/generic/string.h>

namespace NAlice::NMegamind {

inline constexpr TStringBuf DIALOG_ID_DELIMITER = ":";

inline constexpr TStringBuf REQUEST_ID_JSON_KEY = "@request_id";
inline constexpr TStringBuf SCENARIO_NAME_JSON_KEY = "@scenario_name";

inline constexpr TStringBuf SEMANTIC_FRAME_REQUEST_NAME = "@@mm_semantic_frame";

} // namespace NAlice::NMegamind
