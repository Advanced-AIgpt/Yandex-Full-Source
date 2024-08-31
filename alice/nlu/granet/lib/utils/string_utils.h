#pragma once

#include <util/folder/path.h>
#include <util/generic/algorithm.h>
#include <util/string/split.h>
#include <util/string/strip.h>

namespace NGranet {

// ~~~~ Strings ~~~~

inline TString Cite(TStringBuf text) {
    return TString::Join('\"', text, '\"');
}

inline TString Cite(const TString& text) {
    return TString::Join('\"', text, '\"');
}

inline TString Cite(const TFsPath& path) {
    return TString::Join('\"', path.GetPath(), '\"');
}

TString Unquote(TStringBuf str);

bool TryRemoveBraces(TStringBuf* str, char prefix, char suffix);
TStringBuf RemoveBraces(TStringBuf str, char prefix, char suffix);

TString FormatErrorPosition(TStringBuf text, size_t offset, TStringBuf path);

inline const TString TRUE_STR = "true";
inline const TString FALSE_STR = "false";

inline const TString& FormatBool(bool value) {
    return value ? TRUE_STR : FALSE_STR;
}

TUtf16String CropWithEllipsis(TWtringBuf str, size_t width, TWtringBuf ellipsis = u"...");
TString CropWithEllipsis(TStringBuf str, size_t width, TStringBuf ellipsis = TStringBuf("..."));

TUtf16String FitToWidth(TWtringBuf str, size_t width, TWtringBuf ellipsis = u"...", wchar16 padding = u' ');
TString FitToWidth(TStringBuf str, size_t width, TStringBuf ellipsis = "...", char padding = ' ');

TUtf16String LeftJustify(TWtringBuf str, size_t width, wchar16 padding = u' ');
TString LeftJustify(TStringBuf str, size_t width, char padding = ' ');

TUtf16String RightJustify(TWtringBuf str, size_t width, wchar16 padding = u' ');
TString RightJustify(TStringBuf str, size_t width, char padding = ' ');

// Returns vector of stripped strings.
// Examples:
//   StripStrings(StringSplitter(str).Split(',').ToList<TStringBuf>());                   // keep empty tokens
//   StripStrings(StringSplitter(str).Split(',').SkipEmpty().ToList<TStringBuf>(), true); // drop empty tokens
template <class StringType>
TVector<StringType> StripStrings(TVector<StringType> strings, bool dropEmpty = false) {
    for (StringType& string : strings) {
        StripString(string, string);
    }
    if (dropEmpty) {
        ::Erase(strings, StringType());
    }
    return strings;
}

template <class Delimiter>
TVector<TString> SplitAndStrip(TStringBuf str, const Delimiter& delimiter) {
    TVector<TString> result;
    for (const auto& token : StringSplitter(str).Split(delimiter)) {
        result.emplace_back(StripString(token.Token()));
    }
    return result;
}

// ~~~~ Append string by delimiter ~~~~

template <class Delimiter, class SrcStr, class DestStr>
inline void AppendByDelimiter(const Delimiter& delimiter, const SrcStr& src, DestStr* dest) {
    Y_ASSERT(dest);
    if (!dest->empty()) {
        dest->append(delimiter);
    }
    dest->append(src);
}

template <class SrcStr, class DestStr>
inline void AppendBySpace(const SrcStr& src, DestStr* dest) {
    AppendByDelimiter(' ', src, dest);
}

template <class Delimiter, class SrcList, class DestList>
void AppendByDelimiterCartesian(const Delimiter& delimiter, const SrcList& srcList, DestList* destList) {
    Y_ASSERT(destList);
    if (srcList.empty()) {
        destList->clear();
        return;
    }
    if (srcList.size() == 1) {
        for (auto& destStr : *destList) {
            AppendByDelimiter(delimiter, srcList.front(), &destStr);
        }
        return;
    }
    DestList result{Reserve(srcList.size() * destList->size())};
    for (const auto& srcStr : srcList) {
        for (const auto& destStr : *destList) {
            AppendByDelimiter(delimiter, srcStr, &result.emplace_back(destStr));
        }
    }
    *destList = std::move(result);
}

template <class SrcList, class DestList>
inline void AppendBySpaceCartesian(const SrcList& srcList, DestList* destList) {
    AppendByDelimiterCartesian(' ', srcList, destList);
}

} // namespace NGranet
