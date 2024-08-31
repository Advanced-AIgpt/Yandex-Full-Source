#pragma once

#include <alice/boltalka/generative/inference/core/proto/tokenizer_type.pb.h>
#include <alice/boltalka/libs/text_utils/context_transform.h>
#include <dict/mt/libs/nn_base/nn_voc.h>
#include <dict/mt/libs/libmt/token.h>
#include <ml/zeliboba/libs/sentencepiece/cpp/processor.h>

#include <library/cpp/langs/langs.h>

#include <util/generic/string.h>
#include <util/generic/ptr.h>
#include <util/string/builder.h>

#include <memory>

namespace NGenerativeBoltalka {
    class TTokenizer: public TThrRefBase {
    public:
        using TPtr = TIntrusivePtr<TTokenizer>;

        struct TParams {
            ETokenizerType TokenizerType;
            TString BpeVocPath = "";
            TString BpeVocContent = "";
            TString TokenToIdVocPath = "";
            ELanguage Language = LanguageByName("ru");
            TString SeparatorToken = "[SPECIAL_SEPARATOR_TOKEN]";
            int DstEosId = 1;
            int DstBosId = 0;
            size_t MaxLen = 64;
            ETruncatePolicy TruncatePolicy;
            bool ShouldAddSeparatorToken = true;
            bool RemoveHyphensFromInput = true; // deletion of "-" for some models that were not trained with hyphens in datasets
            bool ShouldAddCTRLToken = false;
            bool HasSnippetInContext = false;
            TString CTRLToken = "[PLACEHOLDER] ";
            size_t PTuneTokensNum = 0;
            TString Prefix = "";
            TString AliceName = "Алиса";
            TString UserName = "Пользователь";
            TString TurnSeparator = " [SEP] ";
            TString NameSeparator = ": ";
            TString BeforeInfoText = "";
            TString BeforePrefixText = "\nВопрос: ";
            TString BeforeSuffixText = "\nОтвет:";
        };

        TTokenizer(const TTokenizer::TParams& params);

        virtual void PreprocessContext(TVector<TString>& context) const = 0;
        virtual TVector<int> TokenizeContext(
            const TVector<TString>& context, const size_t reservedTokensNum = 0) const = 0;  // reservedTokensNum is query-wise number by which we should decrease MaxLen
        virtual TVector<int> TokenizeLine(const TString& line) const = 0;
        virtual TString Detokenize(const std::vector<int>& ids) const = 0;
        virtual bool PreventInputOverflow(const TVector<int>& frozenPrefix, const size_t reservedTokensNum,
                                          TVector<int>& flexiblePrefix, bool& lastTokenErased) const;
        virtual int TokenId(const TString& token) const = 0;
        virtual int Unk() const = 0;
        virtual int Eos() const = 0;
        virtual int Bos() const = 0;

        std::unique_ptr<NDict::NMT::IBaseVoc> Voc;

    protected:
        TTokenizer::TParams Params;
    };

    typename TTokenizer::TPtr GetTokenizer(const TTokenizer::TParams& params);

    class TGenerativeTokenizer: public TTokenizer {
    public:
        TGenerativeTokenizer(const TTokenizer::TParams& params);
        virtual void PreprocessContext(TVector<TString>& context) const;
        virtual TVector<int> TokenizeContext(const TVector<TString>& context, const size_t reservedTokensNum = 0) const;
        virtual TVector<int> TokenizeLine(const TString& line) const;
        virtual TString Detokenize(const std::vector<int>& ids) const;
        virtual int TokenId(const TString& token) const;
        virtual int Unk() const;
        virtual int Eos() const;
        virtual int Bos() const;

    private:
        THolder<NDict::NMT::TTokenizer> Tokenizer;
        NNlgTextUtils::TNlgSearchContextTransform ContextTransform;
    public:
        std::unique_ptr<NDict::NMT::IBaseVoc> Voc;
    };

    class TZelibobaTokenizer: public TTokenizer {
    public:
        TZelibobaTokenizer(const TTokenizer::TParams& params);
        virtual void PreprocessContext(TVector<TString>& context) const;
        virtual TVector<int> TokenizeContext(const TVector<TString>& context, const size_t reservedTokensNum = 0) const;
        virtual TVector<int> TokenizeLine(const TString& line) const;
        virtual TString Detokenize(const std::vector<int>& ids) const;
        virtual TStringBuilder GetContextStringBuilder(const TVector<TString>& context) const;
        virtual int TokenId(const TString& token) const;
        virtual int Unk() const;
        virtual int Eos() const;
        virtual int Bos() const;

    private:
        void AddPTuneTokensToVoc();

        NSentencePiece::TSentencePieceTokenizer SentencePieceTokenizer;
    public:
        std::unique_ptr<NDict::NMT::IBaseVoc> Voc;
        TVector<int> PTuneTokensIds;
    };

    class TZelibobaSEPTokenizer: public TZelibobaTokenizer {
    public:
        TZelibobaSEPTokenizer(const TTokenizer::TParams& params);
        virtual TStringBuilder GetContextStringBuilder(const TVector<TString>& context) const;
    };

    class TZelibobaPrefixTokenizer: public TZelibobaTokenizer {
    public:
        TZelibobaPrefixTokenizer(const TTokenizer::TParams& params);
        virtual TStringBuilder GetContextStringBuilder(const TVector<TString>& context) const;
    };

    class TZelibobaSEPNamesTokenizer: public TZelibobaTokenizer {
    public:
        TZelibobaSEPNamesTokenizer(const TTokenizer::TParams& params);
        virtual TStringBuilder GetContextStringBuilder(const TVector<TString>& context) const;
    private:
        bool ShouldAddCTRLToken = false;
        bool HasSnippetInContext = false;
        TString CTRLToken = "[PLACEHOLDER] ";
        TString Prefix = "";
        TString AliceName = "Алиса";
        TString UserName = "Пользователь";
        TString TurnSeparator = " [SEP] ";
        TString NameSeparator = ": ";
    };

    class TZelibobaExternalInfoTokenizer: public TZelibobaTokenizer {
    public:
        TZelibobaExternalInfoTokenizer(const TTokenizer::TParams& params);
        virtual TStringBuilder GetContextStringBuilder(const TVector<TString>& context) const;
    private:
        TString BeforeInfoText = "";
        TString BeforePrefixText = "\nВопрос: ";
        TString BeforeSuffixText = "\nОтвет:";
    };

} // namespace NGenerativeBoltalka
