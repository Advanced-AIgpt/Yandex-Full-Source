#pragma once

#include <util/generic/string.h>

namespace NAlice {

/** Converts alice uuid to search uuid format.
 *  Right now it just removes '-' symbols.
 */
TString ConvertUuidForSearch(TString uuid);

/** Converts alice userAgent to search userAgent format.
 *  Right now it just replace 'Yandex' to 'yandex'.
 */
TString ConvertUserAgentForSearch(TString userAgent);

} // namespace NAlice
