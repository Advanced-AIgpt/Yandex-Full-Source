#include "tokenizer.h"

#include <dict/dictutil/ios.h>
#include <dict/dictutil/str.h>
#include <dict/dictutil/version.h>
#include <dict/mt/libs/nn/voc.h>

#include <util/string/join.h>

namespace NGenerativeBoltalka {
    TTokenizer::TPtr GetTokenizer(const TTokenizer::TParams& params) {
        switch (params.TokenizerType) {
            case ETokenizerType::GENERATIVE: return new TGenerativeTokenizer(params);
            case ETokenizerType::ZELIBOBA: return new TZelibobaTokenizer(params);
            case ETokenizerType::ZELIBOBA_SEP: return new TZelibobaSEPTokenizer(params);
            case ETokenizerType::ZELIBOBA_PREFIX: return new TZelibobaPrefixTokenizer(params);
            case ETokenizerType::ZELIBOBA_SEP_NAMES: return new TZelibobaSEPNamesTokenizer(params);
            case ETokenizerType::ZELIBOBA_EXTERNAL_INFO: return new TZelibobaExternalInfoTokenizer(params);
            default: Y_ENSURE(false);
        }
    }

    namespace {

        THolder<NDict::NMT::TTokenizer> CreateTokenizer(const TTokenizer::TParams& params) {
            TString TokenizerParams = "class=SpaceTokenizer;filter=SegmenterFilter;mode=BPE;file=" + params.BpeVocPath + ";sentinels=1;langSplit=0;filter=UrlFilter";

            const int intVer = NDict::NMT::TTokenizer::GetVersionFromParams(TokenizerParams);
            TVersion Version = TVersion(NDict::NMT::VER_MAJOR(intVer), NDict::NMT::VER_MINOR(intVer));

            NDict::NMT::TTokenizerParams tokenParams;
            tokenParams.Version = NDict::NMT::MK_VER(Version.Major(), Version.Minor());
            tokenParams.SetLang(params.Language, params.Language);
            tokenParams.SetParams(TokenizerParams);

            return THolder<NDict::NMT::TTokenizer>(NDict::NMT::TTokenizer::Create(tokenParams, nullptr));
        }

        std::unique_ptr<NDict::NMT::IBaseVoc> CreateVoc(const TTokenizer::TParams& params) {
            // loading token->id dict
            NDict::NMT::TSimpleVocParams vocParams(params.Language, params.TokenToIdVocPath, 2, 0, 1, false);
            return CreateSimpleVoc(vocParams);
        }

        TVector<TUtf16String> TokenizeString(const TUtf16String& string, const NDict::NMT::TTokenizer* tokenizer) {
            TStringReader in(string);
            std::unique_ptr<NDict::NMT::ITokenInputStream> tokens(tokenizer->Tokenize(&in));

            TVector<TUtf16String> tokenStrings;
            for (NDict::NMT::TToken token; tokens->Read(token);) {
                tokenStrings.push_back(token.Text);
            }

            return tokenStrings;
        }

    } // namespace anonymous

    TTokenizer::TTokenizer(const TTokenizer::TParams& params)
        : Params(params) {};

    bool TTokenizer::PreventInputOverflow(const TVector<int>& frozenPrefix, const size_t reservedTokensNum,
                                          TVector<int>& flexiblePrefix, bool& lastTokenErased) const {
        if (frozenPrefix.size() + flexiblePrefix.size() + reservedTokensNum > Params.MaxLen) {
            auto numToErase = frozenPrefix.size() + flexiblePrefix.size() - Params.MaxLen + reservedTokensNum;
            Y_ENSURE(numToErase < flexiblePrefix.size());
            if (Params.TruncatePolicy == ETruncatePolicy::KeepSuffix) {
                auto startingDeletion = flexiblePrefix.begin();
                flexiblePrefix.erase(startingDeletion, startingDeletion + numToErase);
                lastTokenErased = (numToErase == flexiblePrefix.size());
            } else if (Params.TruncatePolicy == ETruncatePolicy::KeepPrefix) {
                auto startingDeletion = flexiblePrefix.end() - numToErase;
                flexiblePrefix.erase(startingDeletion, flexiblePrefix.end());
                lastTokenErased = true;
            }
            return true;
        }
        lastTokenErased = false;
        return false;
    }

    TGenerativeTokenizer::TGenerativeTokenizer(const TTokenizer::TParams& params)
        : TTokenizer(params)
        , Tokenizer(CreateTokenizer(params))
        , ContextTransform(params.Language)
        , Voc(CreateVoc(params))
    {
    }

    void TGenerativeTokenizer::PreprocessContext(TVector<TString>& context) const {
        if (Params.RemoveHyphensFromInput) {
            for (auto &str : context) {
                ReplaceAll(str, "-", " ");
            }
        }
    }

    TVector<int> TGenerativeTokenizer::TokenizeContext(const TVector<TString>& context, const size_t reservedTokensNum) const {
        Y_UNUSED(reservedTokensNum);
        auto processedContext = ContextTransform.Transform(context);
        TVector<int> inputIds = {(int) Voc->Bos()};

        int separatorTokenId = -1;
        if (Params.ShouldAddSeparatorToken) {
            separatorTokenId = Voc->Id(TUtf16String::FromUtf8(Params.SeparatorToken));
        }

        for (const TString& str : processedContext) {
            if (str.empty()) {
                continue;
            }

            for (const auto& token : TokenizeString(UTF8ToWide(str), Tokenizer.Get())) {
                inputIds.emplace_back(Voc->Id(token));
            }
            if (Params.ShouldAddSeparatorToken) {
                inputIds.push_back(separatorTokenId);
            }
        }
        if (Params.ShouldAddSeparatorToken && inputIds.size() > 1) {
            inputIds.pop_back();  // removing last unnecessary separator token id
        }
        inputIds.emplace_back(Voc->Eos());

        if (inputIds.size() > Params.MaxLen) {
            auto startingDeletion = inputIds.begin() + 1; // + 1 since we do not want to remove "BOS" token
            inputIds.erase(startingDeletion, startingDeletion + (inputIds.size() - Params.MaxLen));
        }

        return inputIds;
    }

    TVector<int> TGenerativeTokenizer::TokenizeLine(const TString& line) const {
        TVector<TUtf16String> tokens = TokenizeString(UTF8ToWide(line), Tokenizer.Get());

        TVector<int> tokenIds;
        EncodeInput(tokens, Voc.get(), &tokenIds);

        return tokenIds;
    }

    TString TGenerativeTokenizer::Detokenize(const std::vector<int>& ids) const {
        TVector<TUtf16String> tokenStrings;

        if (!ids.empty()) {
            for (auto id : ids) {
                if (id == Voc->Eos() || id == Voc->Bos()) {
                    continue;
                }
                tokenStrings.emplace_back(Voc->Word(id));
            }
        }

        TString string = JoinSeq(" ", tokenStrings);
        ReplaceAll(string, " `", "");
        return string;
    }

    int TGenerativeTokenizer::TokenId(const TString& token) const {
        return Voc->Id(TUtf16String::FromUtf8(token));
    }

    int TGenerativeTokenizer::Unk() const {
        return Voc->Unk();
    }

    int TGenerativeTokenizer::Eos() const {
        return Voc->Eos();
    }

    int TGenerativeTokenizer::Bos() const {
        return Voc->Bos();
    }

    TZelibobaTokenizer::TZelibobaTokenizer(const TTokenizer::TParams& params)
        : TTokenizer(params)
        , SentencePieceTokenizer()
    {
        if (params.BpeVocContent) {
            SentencePieceTokenizer.LoadFromSerializedProto(params.BpeVocContent);
        } else {
            SentencePieceTokenizer.LoadFromFile(params.BpeVocPath);
        }
    }

    void TZelibobaTokenizer::PreprocessContext(TVector<TString>& context) const {
        // Nothing to see here
        Y_UNUSED(context);
    }

    TStringBuilder TZelibobaTokenizer::GetContextStringBuilder(const TVector<TString>& context) const {
       TStringBuilder text;
       for (size_t i = 0; i < context.size(); ++i) {
           text << ((context.size() - i) % 2 ? "Пользователь: " : "Алиса: ");
           text << context[i] << "\n";
        }
        text << "Ответ Алисы: ";
        return text;
    }

    TVector<int> TZelibobaTokenizer::TokenizeContext(const TVector<TString>& context, const size_t reservedTokensNum) const {
        auto text = GetContextStringBuilder(context);
        TVector<TString> tokens;
        SentencePieceTokenizer.Encode(text, &tokens);
        TVector<int> inputIds = SentencePieceTokenizer.PieceToId(tokens);

        if (!PTuneTokensIds.empty()) {
            inputIds.insert(inputIds.begin(), PTuneTokensIds.begin(),
                            &PTuneTokensIds[PTuneTokensIds.size() / 2]);
            inputIds.insert(inputIds.end(), &PTuneTokensIds[PTuneTokensIds.size() / 2], PTuneTokensIds.end());
        }
        bool lastTokenErased = false;
        PreventInputOverflow({}, reservedTokensNum + 1, inputIds, lastTokenErased);
        inputIds.insert(inputIds.begin(), Bos());
        return inputIds;
    }

    TVector<int> TZelibobaTokenizer::TokenizeLine(const TString& line) const {
        TVector<TString> inputTokens;
        SentencePieceTokenizer.Encode(line, &inputTokens);

        return SentencePieceTokenizer.PieceToId(inputTokens);
    }

    TString TZelibobaTokenizer::Detokenize(const std::vector<int>& ids) const {
        TVector<TString> tokenStrings;
        if (!ids.empty()) {
            for (auto id : ids) {
                if (id == Eos() || id == Bos()) {
                    continue;
                }
                tokenStrings.emplace_back(SentencePieceTokenizer.IdToPiece({id})[0]);
            }
        }

        TString result;
        SentencePieceTokenizer.Decode(tokenStrings, &result);
        return result;
    }

    int TZelibobaTokenizer::TokenId(const TString& token) const {
        return SentencePieceTokenizer.PieceToId({token})[0];
    }

    int TZelibobaTokenizer::Unk() const {
        return 2;
    }

    int TZelibobaTokenizer::Eos() const {
        return 1;
    }

    int TZelibobaTokenizer::Bos() const {
        return 0;
    }

    TZelibobaSEPTokenizer::TZelibobaSEPTokenizer(const TTokenizer::TParams& params)
        : TZelibobaTokenizer(params)
    {
    }

    TStringBuilder TZelibobaSEPTokenizer::GetContextStringBuilder(const TVector<TString>& context) const {
       TStringBuilder text;
       for (size_t i = 0; i < context.size(); ++i) {
           text << context[i];
           text << " [SEP] ";
        }
       return text;
    }

    TZelibobaPrefixTokenizer::TZelibobaPrefixTokenizer(const TTokenizer::TParams& params)
        : TZelibobaTokenizer(params)
    {
    }

    TStringBuilder TZelibobaPrefixTokenizer::GetContextStringBuilder(const TVector<TString>& context) const {
       Y_ENSURE(context.size() == 1, "Prefix tokenizer expects one text");
       TStringBuilder text;
       text << context[0];
       return text;
    }

    TZelibobaSEPNamesTokenizer::TZelibobaSEPNamesTokenizer(const TTokenizer::TParams& params)
        : TZelibobaTokenizer(params),
        ShouldAddCTRLToken(params.ShouldAddCTRLToken),
        HasSnippetInContext(params.HasSnippetInContext),
        CTRLToken(params.CTRLToken),
        Prefix(params.Prefix),
        AliceName(params.AliceName),
        UserName(params.UserName),
        TurnSeparator(params.TurnSeparator),
        NameSeparator(params.NameSeparator)
    {
    }

    TStringBuilder TZelibobaSEPNamesTokenizer::GetContextStringBuilder(const TVector<TString>& context) const {
        TStringBuilder text;
        size_t start_index = 0;
        text << Prefix;
        if (HasSnippetInContext && context) {
            text << "Подсказка" << NameSeparator << context[0] << TurnSeparator;
            start_index = 1;
        }
        for (size_t i = start_index; i < context.size(); ++i) {
            text << ((context.size() - i) % 2 ? UserName : AliceName);
            text << NameSeparator;
            text << context[i];
            text << TurnSeparator;
        }
        if (ShouldAddCTRLToken) {
            text << CTRLToken;
        }
        text << AliceName << NameSeparator;
        return text;
    }

    TZelibobaExternalInfoTokenizer::TZelibobaExternalInfoTokenizer(const TTokenizer::TParams& params)
        : TZelibobaTokenizer(params),
        BeforeInfoText(params.BeforeInfoText),
        BeforePrefixText(params.BeforePrefixText),
        BeforeSuffixText(params.BeforeSuffixText)
    {
    }

    TStringBuilder TZelibobaExternalInfoTokenizer::GetContextStringBuilder(const TVector<TString>& context) const {
        TStringBuilder text;
        text << BeforeInfoText << context[0] << BeforePrefixText << context[1] << BeforeSuffixText;
        return text;
    }
} // namespace NGenerativeBoltalka
