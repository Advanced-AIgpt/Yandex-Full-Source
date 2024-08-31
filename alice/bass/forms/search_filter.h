#pragma once

#include "vins.h"

namespace NBASS {

namespace NSearchFilter {
inline constexpr TStringBuf SET_LEVEL_FAMILY = "personal_assistant.scenarios.search_filter_set_family";
inline constexpr TStringBuf SET_LEVEL_NO_FILTER = "personal_assistant.scenarios.search_filter_set_no_filter";
inline constexpr TStringBuf SET_LEVEL_MODERATE = "personal_assistant.scenarios.search_filter_reset";
inline constexpr TStringBuf GET_LEVEL = "personal_assistant.scenarios.search_filter_get";
inline constexpr TStringBuf HOW_SET_LEVEL = "personal_assistant.scenarios.search_filter_how";

inline constexpr TStringBuf LEVEL_FAMILY = "strict";
inline constexpr TStringBuf LEVEL_NO_FILTER = "none";
inline constexpr TStringBuf LEVEL_MODERATE = "moderate";
}

class TSearchFilterFormHandler: public IHandler {
public:
    TResultValue Do(TRequestHandler& r) override;

    static void Register(THandlersMap* handlers);
};

}
