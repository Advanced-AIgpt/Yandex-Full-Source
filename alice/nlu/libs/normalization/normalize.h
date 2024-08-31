#pragma once

#include <library/cpp/langs/langs.h>

#include <util/generic/fwd.h>

namespace NNlu {

    TUtf16String NormalizeText(TWtringBuf text, ELanguage lang);
    TString NormalizeText(TStringBuf text, ELanguage lang);

    TUtf16String NormalizeWord(TWtringBuf word, ELanguage lang);
    TString NormalizeWord(TStringBuf word, ELanguage lang);

} // namespace NNlu
