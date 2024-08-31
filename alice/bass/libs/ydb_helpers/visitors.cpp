#include "visitors.h"

#include <alice/bass/libs/logging_v2/logger.h>

#include <util/string/ascii.h>
#include <util/system/types.h>

namespace NYdbHelpers {
namespace {
constexpr TStringBuf HEX_DIGITS = "0123456789ABCDEF";
} // namespace

TString EscapeYQL(TStringBuf s) {
    static_assert(HEX_DIGITS.size() == 16, "Wrong hex digits string");

    TString r;

    for (const ui8 c : s) {
        if (IsAsciiAlnum(c)) {
            r.push_back(c);
            continue;
        }

        const ui8 hi = c >> 4;
        const ui8 lo = c & 0xF;
        Y_ASSERT(hi < HEX_DIGITS.size());
        Y_ASSERT(lo < HEX_DIGITS.size());
        r.append("\\x");
        r.push_back(HEX_DIGITS[hi]);
        r.push_back(HEX_DIGITS[lo]);
    }

    return r;
}
} // namespace NYdbHelpers
