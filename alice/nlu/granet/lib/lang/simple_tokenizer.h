#pragma once

#include <library/cpp/langs/langs.h>
#include <library/cpp/token/nlptypes.h>
#include <library/cpp/tokenizer/tokenizer.h>

namespace NGranet {

// TODO(vl-trifonov) move them in common place
inline const TLangMask GRANET_LANGUAGES = {LANG_RUS, LANG_ENG, LANG_TUR, LANG_KAZ};

// ~~~~ TSimpleTokenizer ~~~~

class TSimpleTokenizer : public TMoveOnly, private ITokenHandler {
public:
    explicit TSimpleTokenizer(TLangMask languages = GRANET_LANGUAGES);

    TString Tokenize(TStringBuf str);
    TUtf16String Tokenize(TWtringBuf str);

private:
    void OnToken(const TWideToken& token, size_t originalLength, NLP_TYPE type) override;

private:
    TLangMask Languages;
    TNlpTokenizer Tokenizer;
    TUtf16String Tokens;
};

} // namespace NGranet
