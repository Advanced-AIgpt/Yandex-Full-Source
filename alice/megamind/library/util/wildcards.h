#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/system/yassert.h>

namespace NAlice {
struct TWildcard {
    enum class EType { Exact, Prefix, Wildcard };

    explicit TWildcard(const TString& pattern);

    bool operator<(const TWildcard& rhs) const {
        // Pattern == rhs.Pattern implies Type == rhs.Type
        Y_ASSERT(Pattern != rhs.Pattern || Type == rhs.Type);
        return Pattern < rhs.Pattern;
    }

    bool operator==(const TWildcard& rhs) const {
        // Pattern == rhs.Pattern implies Type == rhs.Type
        Y_ASSERT(Pattern != rhs.Pattern || Type == rhs.Type);
        return Pattern == rhs.Pattern;
    }

    size_t Size() const { return Pattern.size(); }

    const TString Pattern;
    EType Type;
};

// Matches simple wildcards, where '*' means 'zero or more symbols'.
//
// Complexity: time O(|pattern| * |text|), memory O(|text|).
bool Matches(const TWildcard& pattern, TStringBuf text);
} // namespace NAlice
