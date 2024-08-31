#pragma once

#include <alice/bass/forms/context/context.h>

#include <library/cpp/scheme/scheme.h>

#include <util/generic/vector.h>

namespace NBASS {
namespace NContactsFinder {

const size_t SURNAME_PREFIX_LEN = 5;

inline constexpr TStringBuf PARTIAL_TAG_PREFIX = "partial";
inline constexpr TStringBuf SEARCH_TAG_BASIC = "basic";
inline constexpr TStringBuf SEARCH_TAG_PREFIX = "prefix_search";

TStringBuf GetPossibleSurname(const TSlot& slot, size_t numTokens);
NSc::TValue PrepareClientRequestData(const TSlot* recipient, const TSlot* personalAsr);

bool HandlePermission(TStringBuf permission, TContext& ctx);

} // namespace NContactsFinder
} // namespace NBASS
