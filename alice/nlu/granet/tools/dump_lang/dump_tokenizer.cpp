#include "dump_tokenizer.h"
#include <util/generic/strbuf.h>
#include <util/charset/wide.h>

TNlpTokenizerDumper::TNlpTokenizerDumper(IOutputStream* log)
    : Log(log)
    , Tokenizer(*this, false)
{
    Y_ENSURE(Log);
}

TVector<TUtf16String> TNlpTokenizerDumper::Tokenize(TWtringBuf line, const TTokenizerOptions& options) {
    *Log << "============================================================" << Endl;
    *Log << "\"" << line << "\"" << Endl;
    *Log << "TNlpTokenizer::Tokenize begin" << Endl;

    Tokenizer.Tokenize(line.data(), line.size(), options);

    *Log << "TNlpTokenizer::Tokenize end" << Endl;

    return std::move(Tokens);
}

void TNlpTokenizerDumper::OnToken(const TWideToken& token, size_t originalLength, NLP_TYPE type) {
    DumpOnToken(token, originalLength, type);

    for (const TCharSpan& subToken : token.SubTokens) {
        if (subToken.Type == TOKEN_WORD
            || subToken.Type == TOKEN_NUMBER
            || subToken.Type == TOKEN_FLOAT)
        {
            Tokens.emplace_back(TWtringBuf(token.Token + subToken.Pos, subToken.Len));
        }
    }
}

void TNlpTokenizerDumper::DumpOnToken(const TWideToken& token, size_t originalLength, NLP_TYPE type) {
    *Log << "  \"" << TWtringBuf(token.Token, token.Leng) << "\"" << Endl;
    *Log << "    ITokenHandler::OnToken:" << Endl;
    *Log << "      type: " << ToString(type) << Endl;
    *Log << "      originalLength: " << originalLength << Endl;
    *Log << "      token: " << Endl;
    *Log << "        TWideToken: " << Endl;
    *Log << "          Token: \"" << TWtringBuf(token.Token, token.Leng) << "\"" << Endl;
    *Log << "          Leng: " << token.Leng << Endl;
    *Log << "          SubTokens: " << Endl;
    for (const TCharSpan& subToken : token.SubTokens) {
        *Log << "            TCharSpan: " << Endl;
        *Log << "              SubToken: \"" << TWtringBuf(token.Token + subToken.Pos, subToken.Len) << "\"" << Endl;
        *Log << "              Pos: " << subToken.Pos << Endl;
        *Log << "              Len: " << subToken.Len << Endl;
        *Log << "              PrefixLen: " << subToken.PrefixLen << Endl;
        *Log << "              SuffixLen: " << subToken.SuffixLen << Endl;
        *Log << "              Type: " << ToString(subToken.Type) << Endl;
        *Log << "              Hyphen: " << ToString(subToken.Hyphen) << Endl;
        *Log << "              TokenDelim: " << ToString(subToken.TokenDelim) << Endl;
    }
}
