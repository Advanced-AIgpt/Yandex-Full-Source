#pragma once

#include <util/string/ascii.h>

namespace NAlice {

template <typename CheckType, typename ...RestChecks>
struct TCharChecker {
    static bool Check(const char*& it, const char* const end) {
        for (unsigned i = 0; i < CheckType::RepeatCount; ++i) {
            if (it == end || !CheckType::Check(it))
                return false;
        }
        if constexpr (sizeof...(RestChecks) != 0) {
            return TCharChecker<RestChecks...>::Check(it, end);
        } else {
            return it == end;
        }
    }

    static bool Check(TStringBuf str) {
        const char* begin = str.data();
        return TCharChecker<CheckType, RestChecks...>::Check(begin, begin + str.size());
    }
};

template <unsigned Count>
struct THexDigit {
    static const unsigned RepeatCount = Count;
    static bool Check(const char*& x) {
        return IsAsciiHex(*(x++));
    }
};

template <char Symbol, unsigned Count = 1>
struct TOptional {
    static const unsigned RepeatCount = Count;
    static bool Check(const char*& x) {
        if (*x == Symbol)
            ++x;
        return true;
    }
};

inline bool CheckUuid(TStringBuf str) {
    return TCharChecker<
        THexDigit<8>, TOptional<'-'>,
        THexDigit<4>, TOptional<'-'>,
        THexDigit<4>, TOptional<'-'>,
        THexDigit<4>, TOptional<'-'>,
        THexDigit<12>
    >::Check(str);
}

}  // namespace NAlice
