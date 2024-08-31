#include "string_utils.h"
#include <util/charset/wide.h>
#include <util/stream/mem.h>
#include <util/stream/str.h>
#include <util/string/builder.h>
#include <util/string/escape.h>

namespace NGranet {

// ~~~~ Strings ~~~~

TString Unquote(TStringBuf str) {
    str = StripString(str);
    if (!TryRemoveBraces(&str, '"', '"')) {
        return TString{str};
    }
    return UnescapeC(str);
}

bool TryRemoveBraces(TStringBuf* str, char prefix, char suffix) {
    Y_ASSERT(str);
    if (!str->StartsWith(prefix) || !str->EndsWith(suffix)) {
        return false;
    }
    str->Skip(1);
    str->Chop(1);
    return true;
}

TStringBuf RemoveBraces(TStringBuf str, char prefix, char suffix) {
    Y_ENSURE_EX(TryRemoveBraces(&str, prefix, suffix), TFromStringException());
    return str;
}

static TString GetLastLines(TStringBuf text, size_t n) {
    if (n == 0) {
        return "";
    }
    for (size_t start = text.length(); start > 0; start--) {
        if (text[start - 1] == '\n') {
            n--;
            if (n == 0) {
                return TString(text.SubStr(start));
            }
        }
    }
    return TString(text);
}

TString FormatErrorPosition(TStringBuf text, size_t offset, TStringBuf path) {
    TStringBuf before;
    TStringBuf after;
    text.SplitAt(offset, before, after);
    const size_t lineIndex = ::Count(before, '\n');
    const size_t columnIndex = GetNumberOfUTF8Chars(before.RAfter('\n'));
    TStringBuilder out;
    out << path << ":" << lineIndex + 1 << ":" << columnIndex + 1 << ":" << Endl;
    out << GetLastLines(before, 3) << after.Before('\n') << Endl;
    out << TString(columnIndex, ' ') << "^" << Endl;
    return out;
}

TUtf16String CropWithEllipsis(TWtringBuf str, size_t width, TWtringBuf ellipsis) {
    if (str.length() <= width) {
        return TUtf16String(str);
    }
    if (width <= ellipsis.length()) {
        return TUtf16String(str.Head(width));
    }
    return TUtf16String::Join(str.Head(width - ellipsis.length()), ellipsis);
}

TString CropWithEllipsis(TStringBuf str, size_t width, TStringBuf ellipsis) {
    return WideToUTF8(CropWithEllipsis(UTF8ToWide(str), width, UTF8ToWide(ellipsis)));
}

TUtf16String FitToWidth(TWtringBuf str, size_t width, TWtringBuf ellipsis, wchar16 padding) {
    TUtf16String result = CropWithEllipsis(str, width, ellipsis);
    result.resize(width, padding);
    return result;
}

TString FitToWidth(TStringBuf str, size_t width, TStringBuf ellipsis, char padding) {
    return WideToUTF8(FitToWidth(UTF8ToWide(str), width, UTF8ToWide(ellipsis), padding));
}

TUtf16String LeftJustify(TWtringBuf str, size_t width, wchar16 padding) {
    const size_t length = CountWideChars(str);
    return length >= width ? TUtf16String(str) : str + TUtf16String(width - length, padding);
}

TString LeftJustify(TStringBuf str, size_t width, char padding) {
    const size_t length = GetNumberOfUTF8Chars(str);
    return length >= width ? TString(str) : str + TString(width - length, padding);
}

TUtf16String RightJustify(TWtringBuf str, size_t width, wchar16 padding) {
    const size_t length = CountWideChars(str);
    return length >= width ? TUtf16String(str) : TUtf16String(width - length, padding) + str;
}

TString RightJustify(TStringBuf str, size_t width, char padding) {
    const size_t length = GetNumberOfUTF8Chars(str);
    return length >= width ? TString(str) : TString(width - length, padding) + str;
}

} // namespace NGranet
