#pragma once

#include <library/cpp/scheme/scheme.h>

#include <util/generic/hash.h>
#include <util/generic/vector.h>


namespace NAlice::NIot {

struct TBowToken {
    TString Form;
    bool IsExact;
    TVector<TString> Grammemes;
};

struct TParsedBowToken {
    TStringBuf Form;
    TStringBuf ExactForm;
};

using TBowTokens = TVector<TBowToken>;
using TParsedBowTokens = TVector<TParsedBowToken>;

class TBowIndex {
public:
    TVector<NSc::TValue> ExtractValues(TStringBuf type, TParsedBowTokens tokens) const;
    void Add(TStringBuf type, TBowTokens tokens, const NSc::TValue& value);

private:
    struct TBowEntity {
        NSc::TValue Value;
        TBowTokens SortedTokens;
    };

    bool Matches(const TBowEntity& entity, const TParsedBowTokens& sortedTokens) const;

    template<typename TTokens>
    TString GetKey(TStringBuf type, TTokens& tokens) const;

private:
    THashMap<TString, TVector<TBowEntity>> Data;
};


struct TBowForm {
    TBowTokens Tokens;
};

using TBowForms = TVector<TBowForm>;

/*
 * The input argument is supposed to be described by the following grammar (BNF).
 *
 * <root>            ::= <bow_forms>
 * <bow_forms>       ::= <token> | <token> " " <bow_forms>
 * <token>           ::= "" | <word_form> | <word_exact_form> | <or_token>
 * <word_exact_form> ::= "#" <word_form>
 * <word_form>       ::= <chars> | <chars> "<" <grammemes> ">"
 * <chars>           ::= <sequence of alphabetical characters without spaces>
 * <grammemes>       ::= <chars> | <chars> "," <grammemes>
 * <or_token>        ::= "(" <or_token_body> ")"
 * <or_token_body>   ::= <bow_forms> | <bow_forms> "|" <or_token_body>
 *
 * The list of supported grammemes by the time of writing can be found here:
 * https://a.yandex-team.ru/arc/trunk/arcadia/kernel/lemmer/dictlib/ccl.cpp?rev=5025038#L200.
 * For debug use https://a.yandex-team.ru/arc/trunk/arcadia/tools/lemmer-test.
 */
TBowForms ParseBowForms(TStringBuf raw);

} // namespace NAlice::NIot
