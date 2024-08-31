#pragma once

#include <util/charset/utf8.h>
#include <util/generic/algorithm.h>
#include <util/generic/strbuf.h>
#include <util/string/subst.h>
#include <util/string/strip.h>
#include <util/system/yassert.h>

#include <cstddef>

namespace NBASS {
namespace NSmallGeo {

TString RemoveParentheses(const TString& name);

template <typename TFn>
void Split(TStringBuf s, TStringBuf delims, TFn&& fn) {
    size_t i = 0;
    while (i < s.size()) {
        while (i < s.size() && Find(delims, s[i]) != delims.end())
            ++i;

        size_t j = i;
        while (j < s.size() && Find(delims, s[j]) == delims.end())
            ++j;

        if (i != j)
            fn(s.SubStr(i, j - i));
        else
            Y_ASSERT(i == s.size());

        i = j;
    }
}

template <typename TFn>
void ForEachToken(TStringBuf s, TFn&& fn) {
    constexpr TStringBuf delims = " -,'!.\"`()[]{}|";
    Split(s, delims, [&fn](TStringBuf token) {
        TString t = ToLowerUTF8(token);
        SubstGlobal(t, "ё", "е");
        fn(t);
    });
}

} // namespace NSmallGeo
} // namespace NBASS
