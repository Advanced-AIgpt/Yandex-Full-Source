#pragma once

#include <alice/nlu/libs/interval/interval.h>

#include <library/cpp/langs/langs.h>
#include <library/cpp/token/nlptypes.h>
#include <library/cpp/tokenizer/tokenizer.h>

#include <util/generic/fwd.h>
#include <util/generic/ptr.h>

namespace NNlu {

    class ITokenizer {
    public:
        virtual TVector<TString> GetOriginalTokens() const = 0;
        virtual TVector<TString> GetNormalizedTokens() const = 0;
        virtual ~ITokenizer() {}
    };

    class TTokenizerBase : public TMoveOnly, public ITokenizer {
    public:
        TVector<TString> GetNormalizedTokens() const override;

    protected:
        TTokenizerBase(ELanguage lang)
            : Lang(lang)
        {
        }

    protected:
        ELanguage Lang = LANG_UNK;
    };

    class TSmartTokenizer : public TTokenizerBase, private ITokenHandler {
    public:
        TSmartTokenizer(TStringBuf line, ELanguage lang);

        TVector<TString> GetOriginalTokens() const override;
        TVector<TInterval> GetTokensIntervals() const;

    private:
        void OnToken(const TWideToken& token, size_t originalLength, NLP_TYPE type) override;

    private:
        TString Line;
        TUtf16String WideLine;
        size_t CurrentTokenBegin = 0;
        size_t CurrentTokenEnd = 0;
        TVector<TUtf16String> Tokens;
        TVector<TInterval> IntervalsInWideLine;
    };

    class TWhiteSpaceTokenizer : public TTokenizerBase {
    public:
        TWhiteSpaceTokenizer(TStringBuf line, ELanguage lang);

        TVector<TString> GetOriginalTokens() const override;

    private:
        TVector<TString> Tokens;
    };

    enum class ETokenizerType {
        DEFAULT    /* "default" */,
        SMART    /* "smart" */,
        WHITESPACE /* "whitespace" */,
    };

    inline THolder<ITokenizer> CreateTokenizer(ETokenizerType type, TStringBuf line, ELanguage lang) {
        switch (type) {
            case ETokenizerType::DEFAULT:
            case ETokenizerType::SMART: {
                return MakeHolder<TSmartTokenizer>(line, lang);
            }
            case ETokenizerType::WHITESPACE: {
                return MakeHolder<TWhiteSpaceTokenizer>(line, lang);
            }
        }
    }

} // namespace NNlu
