#include "utils.h"

#include <util/charset/utf8.h>
#include <util/string/subst.h>
#include <util/system/yassert.h>

namespace NAlice::NMisspell {
namespace {

constexpr TStringBuf MISSPELL_MARKUP_OPEN = "\xc2\xad\x28";
constexpr TStringBuf MISSPELL_MARKUP_CLOSE = "\x29\xc2\xad";

} // namespace

TMaybe<TString> CleanMisspellMarkup(TString text) {
    if (Y_UNLIKELY(!IsUtf(text))) {
        return Nothing();
    }
    SubstGlobal(text, MISSPELL_MARKUP_OPEN, "");
    if (Y_UNLIKELY(!IsUtf(text))) {
        return Nothing();
    }
    SubstGlobal(text, MISSPELL_MARKUP_CLOSE, "");
    if (IsUtf(text)) {
        return text;
    }
    return Nothing();
}

} // namespace NAlice::NMisspell
