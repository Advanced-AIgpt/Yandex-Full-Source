#include "wildcards.h"

#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/generic/ylimits.h>
#include <util/stream/output.h>

#include <algorithm>

namespace NAlice {
namespace {
bool Matches(TStringBuf pattern, TStringBuf text) {
    const size_t n = pattern.size();
    const size_t m = text.size();

    Y_ENSURE(n != Max<size_t>());
    Y_ENSURE(m != Max<size_t>());

    TVector<bool> curr(m + 1);
    TVector<bool> next(m + 1);

    curr[0] = true;
    if (pattern.StartsWith('*')) {
        std::fill(curr.begin(), curr.end(), true);
    }

    for (size_t i = 1; i <= n; ++i) {
        bool prefix = curr[0];

        next[0] = curr[0] && pattern[i - 1] == '*';
        for (size_t j = 1; j <= m; ++j) {
            prefix = prefix || curr[j];

            if (pattern[i - 1] == '*') {
                next[j] = prefix;
            } else {
                next[j] = pattern[i - 1] == text[j - 1] && curr[j - 1];
            }
        }
        curr.swap(next);
    }

    return curr[m];
}
} // namespace

// TWildcard -------------------------------------------------------------------
TWildcard::TWildcard(const TString& pattern)
    : Pattern(pattern)
    , Type(EType::Wildcard)
{
    const auto star = pattern.find_first_of('*');
    if (star == TStringBuf::npos) {
        Type = EType::Exact;
    } else if (star + 1 == pattern.size()) {
        Type = EType::Prefix;
    }
}

// -----------------------------------------------------------------------------
bool Matches(const TWildcard& pattern, TStringBuf text) {
    switch (pattern.Type) {
        case TWildcard::EType::Exact:
            return pattern.Pattern == text;
        case TWildcard::EType::Prefix:
            return text.StartsWith(TStringBuf{pattern.Pattern}.Chop(1));
        case TWildcard::EType::Wildcard:
            return Matches(pattern.Pattern, text);
    }
}
} // namespace NAlice

template <>
void Out<NAlice::TWildcard>(IOutputStream& os, const NAlice::TWildcard& wildcard) {
    os << "TWildcard [" << wildcard.Pattern << "]";
}
