#include "contacts_custom_entity.h"

#include <alice/nlu/libs/tokenization/tokenizer.h>

#include <alice/library/contacts/contacts.h>

#include <util/charset/wide.h>
#include <util/generic/mapfindptr.h>
#include <util/generic/vector.h>
#include <util/string/builder.h>
#include <util/string/join.h>

namespace NAlice::NContacts {

namespace {

const static TString ENTITY_TYPE = "device.address_book.item_name";

constexpr double PENALTY_0 = 0;
constexpr double PENALTY_1 = -5.0;
constexpr double PENALTY_2 = -10.0;
constexpr double PENALTY_3 = -15.0;

struct TMatchingInfo {
    TString Key;
    TVector<TString> Tokens;
};

using TAddVariantsFunc = std::function<void(const TVector<TString>& tokens, const TString& value, double penalty, TVector<NNlu::TEntityString>& variants)>;

TVector<TString> GetTokens(TStringBuf text, ELanguage lang) {
    ::NNlu::TSmartTokenizer tokenizer(text, lang);
    auto tokens = tokenizer.GetNormalizedTokens();
    if (tokens.size() > MAX_CONSIDERED_TOKENS_SIZE) {
        tokens.resize(MAX_CONSIDERED_TOKENS_SIZE);
    }

    return tokens;
}

TVector<TString> GetPermutations(const TVector<TString>& tokensOriginal) {
    if (tokensOriginal.size() > 4) {
        return { JoinSeq(" ", tokensOriginal) };
    }

    TVector<TString> tokens(tokensOriginal);
    TVector<TString> result;
    std::sort(tokens.begin(), tokens.end());
    do {
        result.push_back(JoinSeq(" ", tokens));
    } while (std::next_permutation(tokens.begin(), tokens.end()));

    return result;
}

bool IsTokenContainsOnlyAlpha(const TUtf16String& token) {
    for (const auto& ch : token) {
        if (!IsAlpha(ch)) {
            return false;
        }
    }

    return true;
}

void AddVariantsOneToken(const TVector<TString>& tokens, const TString& value, double penalty, TVector<NNlu::TEntityString>& variants) {
    for (const auto& token : tokens) {
        const auto tokenWide = UTF8ToWide(token);
        if (CountWideChars(tokenWide) <= 2 && IsTokenContainsOnlyAlpha(tokenWide)) {
            continue;
        }

        variants.push_back({.Sample = token, .Type = ENTITY_TYPE, .Value = value, .LogProbability = penalty});
    }
}

void AddVariantsAllTokens(const TVector<TString>& tokens, const TString& value, double penalty, TVector<NNlu::TEntityString>& variants) {
    for (const auto& permutation : GetPermutations(tokens)) {
        variants.push_back({.Sample = permutation, .Type = ENTITY_TYPE, .Value = value, .LogProbability = penalty});
    }
}

void AddVariantsTwoTokens(const TVector<TString>& tokens, const TString& value, double penalty, TVector<NNlu::TEntityString>& variants) {
    for (size_t i = 0; i < tokens.size(); ++i) {
        for (size_t j = i + 1; j < tokens.size(); ++j) {
            variants.push_back({.Sample = TString::Join(tokens[i], " ", tokens[j]), .Type = ENTITY_TYPE, .Value = value, .LogProbability = penalty});
            variants.push_back({.Sample = TString::Join(tokens[j], " ", tokens[i]), .Type = ENTITY_TYPE, .Value = value, .LogProbability = penalty});
        }
    }
}

void AddVariantsMinusOneToken(const TVector<TString>& tokens, const TString& value, double penalty, TVector<NNlu::TEntityString>& variants) {
    for (size_t i = 0; i < tokens.size(); ++i) {
        TVector<TString> tokens2(Reserve(tokens.size() - 1));
        for (size_t j = 0; j < tokens.size(); ++j) {
            if (i != j) {
                tokens2.push_back(tokens[j]);
            }
        }
        for (const auto& permutation : GetPermutations(tokens2)) {
            variants.push_back({.Sample = permutation, .Type = ENTITY_TYPE, .Value = value, .LogProbability = penalty});
        }
    }
}

void AddMoreVariants(const TVector<TMatchingInfo>& matchingInfos, TAddVariantsFunc addVariantsFunc, double penalty, TVector<NNlu::TEntityString>& variants) {
    for (const auto& [key, tokens] : matchingInfos) {
        if (variants.size() >= MAX_CONSIDERED_VARIANTS) {
            break;
        }
        if (tokens.size() > 3) {
            addVariantsFunc(tokens, key, penalty, variants);
        }
    }
}

} // namespace

TVector<NNlu::TEntityString> ParseContacts(const NAlice::TClientEntity& contacts, ELanguage lang) {
    const size_t maximumContactsSize = std::min(static_cast<size_t>(NAlice::NContacts::CONTACTS_ADDRESS_BOOK_MAX_SIZE), contacts.GetItems().size());
    TVector<TMatchingInfo> matchingInfos(Reserve(maximumContactsSize));
    for (const auto& [key, nluHint] : contacts.GetItems()) {
        for (const auto& instance : nluHint.GetInstances()) {
            matchingInfos.push_back({key, GetTokens(instance.GetPhrase(), lang)});
            if (matchingInfos.size() >= maximumContactsSize) {
                break;
            }
        }
    }

    TVector<NNlu::TEntityString> variants;
    for (const auto& [key, tokens] : matchingInfos) {
        if (tokens.size() == 1) {
            AddVariantsOneToken(tokens, key, PENALTY_0, variants);
        } else if (tokens.size() == 2) {
            AddVariantsAllTokens(tokens, key, PENALTY_0, variants);
            AddVariantsOneToken(tokens, key, PENALTY_1, variants);
        } else if (tokens.size() == 3) {
            AddVariantsAllTokens(tokens, key, PENALTY_0, variants);
            AddVariantsTwoTokens(tokens, key, PENALTY_1, variants);
            AddVariantsOneToken(tokens, key, PENALTY_2, variants);
        }
    }

    AddMoreVariants(matchingInfos, AddVariantsAllTokens, PENALTY_0, variants);
    AddMoreVariants(matchingInfos, AddVariantsOneToken, PENALTY_3, variants);
    AddMoreVariants(matchingInfos, AddVariantsTwoTokens, PENALTY_2, variants);
    AddMoreVariants(matchingInfos, AddVariantsMinusOneToken, PENALTY_1, variants);

    return variants;
}

} // namespace NAlice::NContacts
