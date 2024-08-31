#pragma once

#include <util/generic/string.h>

namespace NNlu {

    TUtf16String NormalizeArabicText(TWtringBuf text);
    TString NormalizeArabicText(TStringBuf text);

    namespace NImpl {

        TString NormalizeUnicode(TStringBuf text);
        TString Dediac(TStringBuf text);
        TString NormalizeAlef(TStringBuf text);
        TString NormalizeTehMarbuta(TStringBuf text);
    
    } // namespace NImpl

} // namespace NNlu
