#pragma once

#include "entity.h"
#include "markup.h"
#include <library/cpp/json/writer/json_value.h>
#include <library/cpp/langs/langs.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NGranet {

class TSample : public TThrRefBase {
public:
    using TRef = TIntrusivePtr<TSample>;
    using TConstRef = TIntrusiveConstPtr<TSample>;

public:
    static TRef Create() {
        return new TSample();
    }
    static TRef Create(TStringBuf text, ELanguage lang) {
        return new TSample(text, lang);
    }
    static TRef CreateFromTokens(const TVector<TString>& tokens, ELanguage lang) {
        return new TSample(tokens, lang);
    }
    static TRef Create(TStringBuf text, ELanguage lang, TVector<TString> tokens, TVector<NNlu::TInterval> intervals);

    TRef Copy() const;

    ELanguage GetLanguage() const {
        return Lang;
    }

    // Original text without any normalizations.
    const TString& GetText() const {
        return Text;
    }

    // Tokenized and normalized Text.
    // Normalizations: lower case, remove diacritics.
    const TVector<TString>& GetTokens() const {
        return Tokens;
    }

    // Tokens joined by space
    const TString& GetJoinedTokens() const {
        return JoinedTokens;
    }

    // Positions of Tokens in Text (indexes of bytes in utf-8 representation).
    const TVector<NNlu::TInterval>& GetTokensIntervals() const {
        return TokensIntervals;
    }

    // Lemmatized tokens. For optimization. Not saved to json.
    // Example:
    //   JoinedTokens:  "прилетела сорока воровка"
    //   BestLemmas:    {"прилетать", "сорок", "воровка"}
    //   SureLemmas:    {"прилетать", "сорок,сорока", "воровка"}

    // Best lemma of each token.
    const TVector<TString>& GetBestLemmas() const {
        return BestLemmas;
    }

    // Each item is list (joined by comma) of sure variants of lemmatization corresponded token.
    const TVector<TString>& GetSureLemmas() const {
        return SureLemmas;
    }

    void AddEntityOnTokens(TEntity entity);
    void AddEntityOnText(TEntity entity);

    void AddEntitiesOnTokens(const TVector<TEntity>& entities);

    const TVector<TEntity>& GetEntities() const {
        return Entities;
    }

    // Convert interval on text to interval on tokens and vice versa.
    NNlu::TInterval ConvertPositionToTokens(const NNlu::TInterval& interval) const;
    NNlu::TInterval ConvertPositionToText(const NNlu::TInterval& interval) const;

    bool LoadEntitiesFromJson(const NJson::TJsonValue& json);
    NJson::TJsonValue SaveEntitiesToJson() const;

    NJson::TJsonValue SaveToJsonValue() const;
    TString SaveToJsonString() const;
    static TRef CreateFromJsonValue(const NJson::TJsonValue& json, ELanguage lang);
    static TRef CreateFromJsonString(const TString& string, ELanguage lang);

    static void Tokenize(TStringBuf text, ELanguage lang, TVector<TString>* tokens,
        TVector<NNlu::TInterval>* tokensIntervals);

    TSampleMarkup GetEntitiesAsMarkup(TStringBuf filter, bool alwaysPositive);

    void Dump(IOutputStream* log, const TString& indent = "") const;
    TString PrintMaskedTokens(const NNlu::TInterval& interval) const;
    void DumpEntities(const TVector<TEntity>& entities, IOutputStream* log, const TString& indent = "") const;
    TString GetTextByIntervalOnTokens(const NNlu::TInterval& interval) const;

private:
    TSample();
    TSample(const TSample& other);
    TSample(TStringBuf text, ELanguage lang);
    TSample(const TVector<TString>& tokens, ELanguage lang);
    TSample(TStringBuf text, ELanguage lang, TVector<TString> tokens, TVector<NNlu::TInterval> intervals);
    TSample(const NJson::TJsonValue& json, ELanguage lang);

    void InitTokens();
    void InitHelpers();
    void InitLemmas();
    void InitMaps();
    void Load(const NJson::TJsonValue& json, ELanguage lang);
    bool TryLoadTokensCompact(const NJson::TJsonValue& json);
    bool TryLoadTokensBegemot(const NJson::TJsonValue& json);

private:
    ELanguage Lang = LANG_UNK;
    TString Text;
    TVector<TString> Tokens;
    TString JoinedTokens;
    TVector<NNlu::TInterval> TokensIntervals;
    TVector<TEntity> Entities;

    // Lemmatized tokens. For optimization. Not saved to json.
    // Example:
    //   JoinedTokens:  "прилетела сорока воровка"
    //   BestLemmas:    {"прилетать", "сорок", "воровка"}
    //   SureLemmas:    {"прилетать", "сорок,сорока", "воровка"}
    TVector<TString> BestLemmas;
    TVector<TString> SureLemmas;

    // Maps used for convert interval on Text to interval on Tokens.
    // Result phrase contains token then and only then source phrase contains (not just overlaps) source token.
    // Literally: begin/end positions of phrase in Text -> begin/end of phrase in Tokens.
    TVector<int> MapForBegins;
    TVector<int> MapForEnds;
};

} // namespace NGranet
