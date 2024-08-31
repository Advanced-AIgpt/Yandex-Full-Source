#pragma once

#include <library/cpp/token/nlptypes.h>
#include <library/cpp/tokenizer/tokenizer.h>

class TNlpTokenizerDumper : private ITokenHandler {
public:
    explicit TNlpTokenizerDumper(IOutputStream* log);

    TVector<TUtf16String> Tokenize(TWtringBuf line, const TTokenizerOptions& options = {});

private:
    IOutputStream* Log = nullptr;
    TNlpTokenizer Tokenizer;
    TVector<TUtf16String> Tokens;

private:
    void OnToken(const TWideToken& token, size_t originalLength, NLP_TYPE type) override;
    void DumpOnToken(const TWideToken& token, size_t originalLength, NLP_TYPE type);
};
