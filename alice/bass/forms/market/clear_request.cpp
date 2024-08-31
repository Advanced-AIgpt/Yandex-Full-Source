#include "clear_request.h"

#include <util/charset/utf8.h>
#include <util/generic/vector.h>
#include <util/string/strip.h>
#include <util/string/subst.h>

namespace NBASS {

namespace NMarket {

namespace {

const TVector<std::pair<TString, TString>> COMMON_REPLACE_PHRASES {
    {"айфоны", "apple iphone"},
    {"айфона", "apple iphone"},
    {"айфон", "apple iphone"},
    {"айпады", "apple ipad"},
    {"айпада", "apple ipad"},
    {"айпад", "apple ipad"},
    {"айпэды", "apple ipad"},
    {"айпэда", "apple ipad"},
    {"айпад", "apple ipad"},
    {"эпл", "apple"},
};

} // namespace

TString ClearRequest(
    TStringBuf original,
    const TVector<TString>& removePhrases)
{
    return ClearRequests(original, removePhrases, COMMON_REPLACE_PHRASES);
}

TString ClearRequests(
    TStringBuf original,
    const TVector<TString>& removePhrases,
    const TVector<std::pair<TString, TString>>& replacePhrases)
{
    TString request = ToLowerUTF8(original);
    for (const auto& phrase : removePhrases) {
        SubstGlobal(request, phrase, "");
    }
    for (const auto& repl : replacePhrases) {
        SubstGlobal(request, repl.first, repl.second);
    }
    return StripInPlace(request);
}

} // namespace NMarket

} // namespace NBASS
