#include "simple_tokenizer.h"

namespace NGranet {

// ~~~~ TSimpleTokenizer ~~~~

TSimpleTokenizer::TSimpleTokenizer(TLangMask languages)
    : Languages(std::move(languages))
    , Tokenizer(*this, false)
{
}

TString TSimpleTokenizer::Tokenize(TStringBuf str) {
    return WideToUTF8(Tokenize(UTF8ToWide(str)));
}

TUtf16String TSimpleTokenizer::Tokenize(TWtringBuf str) {
    Tokens.clear();
    Tokenizer.Tokenize(str, false, Languages);
    return std::move(Tokens);
}

void TSimpleTokenizer::OnToken(const TWideToken& token, size_t, NLP_TYPE) {
    for (const TCharSpan& subToken : token.SubTokens) {
        const TWtringBuf exact(token.Token + subToken.Pos, subToken.Len);
        if (!Tokens.empty()) {
            Tokens.append(' ');
        }
        Tokens.append(exact);
    }
}

} // namespace NGranet
