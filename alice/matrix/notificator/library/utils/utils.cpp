#include "utils.h"

#include <util/string/cast.h>

namespace NMatrix::NNotificator {

TExpected<ui64, TString> TryParseFromString(const TString& str, const TString& name) {
    try {
        return FromString<ui64>(str);
    } catch (...) {
        return TString::Join(
            "Unable to cast ", name, " '", str,
            "' to ui64: ", CurrentExceptionMessage()
        );
    }
}

} // namespace NMatrix::NNotificator
