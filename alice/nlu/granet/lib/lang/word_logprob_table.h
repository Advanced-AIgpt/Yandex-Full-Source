#pragma once

#include <library/cpp/langs/langs.h>
#include <util/generic/strbuf.h>

namespace NGranet::NWordLogProbTable {

    float IsLanguageSupported(ELanguage lang);

    float GetWordLogProb(bool isLemma, TStringBuf word, ELanguage lang);

} // namespace NGranet::NWordLogProbTable
