#pragma once

#include "text.h"

#include <library/cpp/json/json_value.h>

namespace NAlice::NNlg {

struct TPhraseOutput {
    TString Text;
    TString Voice;
};

NJson::TJsonValue PostprocessCard(const TText& text, const bool reduceWhitespace = false);
TPhraseOutput PostprocessPhrase(const TText& text);

namespace NPrivate {

// these functions shouldn't be used directly,
// they are only exposed for the purpose of testing
bool IsSpecialPunct(TStringBuf str);
TString CollapsePunctWhitespace(TStringBuf str);
TString UnquoteNewlines(TStringBuf str);
TString PostprocessPhraseString(TStringBuf str);

}  // namespace NPrivate

}  // namespace NAlice::NNlg
