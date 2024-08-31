#include "bow.h"

#include <kernel/lemmer/core/language.h>
#include <kernel/lemmer/dictlib/grammar_index.h>

#include <util/generic/algorithm.h>
#include <util/string/cast.h>
#include <util/string/join.h>


namespace NAlice::NIot {

namespace {

TBowForm operator+(const TBowForm& l, const TBowForm& r) {
    auto newTokens = l.Tokens;
    newTokens.insert(newTokens.end(), r.Tokens.cbegin(), r.Tokens.cend());

    return {newTokens};
}

struct TBowFormCollection {
    TBowForms BowForms;

    TBowFormCollection& operator|=(const TBowFormCollection& other) {
        BowForms.insert(BowForms.end(), other.BowForms.cbegin(), other.BowForms.cend());
        return *this;
    }

    TBowFormCollection operator&(const TBowFormCollection& other) const {
        TBowForms result;

        for (const auto& bf1 : BowForms) {
            for (const auto& bf2 : other.BowForms) {
                result.push_back(bf1 + bf2);
            }
        }

        return {std::move(result)};
    }
};

template<typename TContainer, typename TGetStringF>
TVector<TString> ToStrings(const TContainer& c, TGetStringF f) {
    TVector<TString> result;
    for (const auto& item : c) {
        result.push_back(ToString(f(item)));
    }
    return result;
}

TBowToken ExtractToken(TStringBuf rawToken) {
    TVector<TString> grammemes;
    bool isExact = rawToken.StartsWith("#");
    auto parts = StringSplitter(rawToken.StartsWith("#") ? rawToken.SubStr(1) : rawToken).Split('<').ToList<TString>();
    if (parts.size() == 2) {
        if (parts[1].find('>') != parts[1].size() - 1) {
            ythrow yexception() << "Expected to see exactly one right bracket '>' in \"" << parts[1] << "\"" << Endl;
        }
        grammemes = StringSplitter(parts[1].substr(0, parts[1].size() - 1)).Split(',').ToList<TString>();
    } else if (parts.size() != 1) {
        ythrow yexception() << "Expected to see exactly one left bracket '<' in \"" << rawToken << "\"" << Endl;
    }

    return {std::move(parts[0]), isExact, std::move(grammemes)};
}

bool IsSpecial(const char c) {
    return c == '(' || c == ')' || c == ' ' || c == '|';
}

TBowFormCollection ParseAnd(TStringBuf raw, size_t& i);

TBowFormCollection ParseOr(TStringBuf raw, size_t& i) {
    TBowFormCollection result = ParseAnd(raw, i);

    while (i < raw.size() && raw[i] == '|') {
        result |= ParseAnd(raw, ++i);
    }

    if (i >= raw.size() || raw[i] != ')') {
        ythrow yexception() << "Expected to see ) on " << i << " (byte) position in \"" << raw << "\"" << Endl;
    }

    ++i;
    return result;
}

TBowFormCollection ParseToken(TStringBuf raw, size_t& i) {
    if (i < raw.size() && raw[i] == '(') {
        return ParseOr(raw, ++i);
    }

    const size_t start = i;
    while (i < raw.size() && !IsSpecial(raw[i])) {
        ++i;
    }

    if (i == start) {
        TBowForm emptyBowForm{{}};
        return {{emptyBowForm}};
    }

    return {{{{ExtractToken(raw.SubStr(start, i - start))}}}};
}

TBowFormCollection ParseAnd(TStringBuf raw, size_t& i) {
    TBowFormCollection result = ParseToken(raw, i);

    while(i < raw.size() && raw[i] == ' ') {
        result = result & ParseToken(raw, ++i);
    }

    return result;
}

bool CheckGrammemes(const TYandexLemma& lemma, const TVector<TString>& grammemes) {
    for (const auto& grammeme : grammemes) {
        bool inverted = grammeme.StartsWith("!");
        if (inverted == lemma.HasGram(TGrammarIndex::GetCode(inverted ? grammeme.substr(1) : grammeme))) {
            return false;
        }
    }
    return true;
}

bool CheckGrammemes(const TParsedBowToken& parsedToken, const TBowToken& token) {
    if (token.Grammemes.empty()) {
        return true;
    }

    TWLemmaArray lemmas;
    auto wideParsedToken = UTF8ToWide(parsedToken.ExactForm);
    NLemmer::AnalyzeWord(wideParsedToken.data(), wideParsedToken.size(), lemmas, TLangMask(LANG_RUS));
    for (const auto& lemma : lemmas) {
        if (CheckGrammemes(lemma, token.Grammemes)) {
            return true;
        }
    }

    return false;
}

} // namespace

template<typename TTokens>
TString TBowIndex::GetKey(TStringBuf type, TTokens& tokens) const {
    Sort(tokens.begin(), tokens.end(), [](const auto& l, const auto& r) {
        return l.Form < r.Form;
    });
    auto keyParts = ToStrings(tokens, [](const auto& t) { return t.Form; });
    keyParts.push_back(TString(type));
    return JoinSeq(" ", keyParts);
}

bool TBowIndex::Matches(const TBowEntity& entity, const TParsedBowTokens& sortedTokens) const {
    if (entity.SortedTokens.size() != sortedTokens.size()) {
        return false;
    }

    for (size_t i = 0; i < sortedTokens.size(); ++i) {
        if (!CheckGrammemes(sortedTokens[i], entity.SortedTokens[i])) {
            return false;
        }
        if (entity.SortedTokens[i].IsExact && entity.SortedTokens[i].Form != sortedTokens[i].ExactForm) {
            return false;
        }
    }

    return true;
}

TVector<NSc::TValue> TBowIndex::ExtractValues(TStringBuf type, TParsedBowTokens tokens) const {
    TVector<NSc::TValue> results;

    auto key = GetKey(type, tokens);
    auto entities = Data.FindPtr(key);
    if (!entities) {
        return results;
    }

    for (const auto& entity : *entities) {
        if (Matches(entity, tokens)) {
            results.push_back(entity.Value);
        }
    }

    return results;
}

void TBowIndex::Add(TStringBuf type, TBowTokens tokens, const NSc::TValue& value) {
    auto key = GetKey(type, tokens);
    Data[key].push_back({value, std::move(tokens)});
}

TBowForms ParseBowForms(TStringBuf raw) {
    size_t i = 0;

    auto result = ParseAnd(raw, i);
    if (i != raw.size()) {
        ythrow yexception() << "Finished parsing on " << i << " (byte) position instead of " << raw.size() << " for \"" << raw << "\"" << Endl;
    }

    return std::move(result.BowForms);
}

} // namespace NAlice::NIot
