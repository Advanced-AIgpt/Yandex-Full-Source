#include "contacts_granet.h"

#include <alice/nlu/libs/tokenization/tokenizer.h>

#include <util/generic/mapfindptr.h>
#include <util/generic/vector.h>
#include <util/string/builder.h>
#include <util/string/join.h>

namespace NAlice::NContacts {

namespace {

constexpr TStringBuf HEADER = R"(
entity device.address_book.item_name:
 keep_overlapped: true
 enable_synonyms: true
 keep_variants: true
 values:
)";

constexpr TStringBuf DOUBLE_INDENT = "  ";
constexpr TStringBuf MAIN_TEXT_PATH = "Main";
constexpr TStringBuf PENALTY_0 = "";
constexpr TStringBuf PENALTY_1 = "$p1 ";
constexpr TStringBuf PENALTY_2 = "$p2 ";
constexpr TStringBuf PENALTY_3 = "$p3 ";

TStringBuf GetWordRef(const TString& word, THashMap<TString, TString>& wordsMap) {
    if (const auto wordPtr = MapFindPtr(wordsMap, word)) {
        return *wordPtr;
    }

    const auto it = wordsMap.emplace(word, TString::Join("$w", ToString(wordsMap.size())));
    return it.first->second;
}

TVector<TStringBuf> GetTokens(TStringBuf text, ELanguage lang, THashMap<TString, TString>& wordsMap) {
    NNlu::TSmartTokenizer tokenizer(text, lang);
    const auto normalizedTokens = tokenizer.GetNormalizedTokens();
    TVector<TStringBuf> result(Reserve(normalizedTokens.size()));
    for (const auto& token : normalizedTokens) {
        result.push_back(GetWordRef(token, wordsMap));
    }
    return result;
}

void AddVariantsOneToken(const TVector<TStringBuf>& tokens, TStringBuf penalty, TVector<TString>& variants) {
    for (const auto& token : tokens) {
        variants.push_back(TString::Join(penalty, token));
    }
}

void AddVariantsAllTokens(const TVector<TStringBuf>& tokens, TStringBuf penalty, TVector<TString>& variants) {
    variants.push_back(TString::Join(penalty, "[", JoinSeq(" ", tokens), "]"));
}

void AddVariantsTwoTokens(const TVector<TStringBuf>& tokens, TStringBuf penalty, TVector<TString>& variants) {
    for (size_t i = 0; i < tokens.size(); ++i) {
        for (size_t j = i + 1; j < tokens.size(); ++j) {
            variants.push_back(TString::Join(penalty, "[", tokens[i], " ", tokens[j], "]"));
        }
    }
}

void AddVariantsMinusOneToken(const TVector<TStringBuf>& tokens, TStringBuf penalty, TVector<TString>& variants) {
    for (size_t i = 0; i < tokens.size(); ++i) {
        variants.push_back(
            TString::Join(
                penalty,
                "[",
                JoinRange(" ", tokens.begin(), tokens.begin() + i),
                (i == 0 || i == tokens.size() - 1) ? "" : " ",
                JoinRange(" ", tokens.begin() + i + 1, tokens.end()),
                "]"
            )
        );
    }
}

TMaybe<TString> PreprocessValue(const NAlice::TNluHint& nluHint, ELanguage lang, THashMap<TString, TString>& wordsMap) {
    TVector<TString> variants;
    for (const auto& instance : nluHint.GetInstances()) {
        const auto tokens = GetTokens(instance.GetPhrase(), lang, wordsMap);
        if (tokens.empty()) {
            continue;
        } else if (tokens.size() == 1) {
            AddVariantsOneToken(tokens, PENALTY_0, variants);
        } else if (tokens.size() == 2) {
            AddVariantsAllTokens(tokens, PENALTY_0, variants);
            AddVariantsOneToken(tokens, PENALTY_1, variants);
        } else if (tokens.size() == 3) {
            AddVariantsAllTokens(tokens, PENALTY_0, variants);
            AddVariantsTwoTokens(tokens, PENALTY_1, variants);
            AddVariantsOneToken(tokens, PENALTY_2, variants);
        } else {
            AddVariantsAllTokens(tokens, PENALTY_0, variants);
            AddVariantsMinusOneToken(tokens, PENALTY_1, variants);
            AddVariantsTwoTokens(tokens, PENALTY_2, variants);
            AddVariantsOneToken(tokens, PENALTY_3, variants);
        }
    }

    if (variants.empty()) {
        return Nothing();
    }

    return JoinSeq("; ", variants);
}

} // namespace

TMaybe<NGranet::NCompiler::TSourceTextCollection> ParseContacts(const NAlice::TClientEntity& contacts, ELanguage lang, bool skipExact) {
    if (contacts.GetItems().empty()) {
        return Nothing();
    }

    TStringBuilder builder;
    builder << HEADER;
    bool contactsDefined = false;
    THashMap<TString, TString> wordsMap;
    for (const auto& [key, nluHint] : contacts.GetItems()) {
        const auto value = PreprocessValue(nluHint, lang, wordsMap);
        if (!value.Defined()) {
            continue;
        }
        contactsDefined = true;
        builder << DOUBLE_INDENT << "\"" << key << "\": " << value.GetRef() << '\n';
    }
    if (!contactsDefined) {
        return Nothing();
    }

    for (const auto& [key, value] : wordsMap) {
        builder << value << ": ";
        if (!skipExact) {
            builder << key << "; ";
        }
        builder << "%lemma_as_is; " << key << '\n';
    }
    builder << "$p1: 11111111111111; %weight 0.01; $sys.void\n";
    builder << "$p2: 11111111111111; %weight 0.0001; $sys.void\n";
    builder << "$p3: 11111111111111; %weight 0.000001; $sys.void\n";

    NGranet::NCompiler::TSourceTextCollection result;
    result.MainTextPath = MAIN_TEXT_PATH;
    result.Texts[MAIN_TEXT_PATH] = builder;
    result.ExternalSource = ToString(EXTERNAL_SOURCE_CONTACTS);

    return result;
}

} // namespace NAlice::NContacts
