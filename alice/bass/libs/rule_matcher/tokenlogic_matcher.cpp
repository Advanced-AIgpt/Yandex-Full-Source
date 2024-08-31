#include "tokenlogic_matcher.h"

#include <library/cpp/json/json_writer.h>

#include <util/string/builder.h>
#include <util/string/util.h>

namespace NBASS {

namespace NMatcher {

NSc::TValue TMatchResult::ConvertToJson() const {
    NSc::TValue output;

    output["rule_name"].SetString(RuleName);
    output["tokens"].SetArray();

    for (const auto& token : Tokens) {
        NSc::TValue& t = output["tokens"].Push();
        t["source_text"].SetString(WideToUTF8(token.Text));
        t["type"].SetString(token.Type);
        t["lemma"].SetString(token.Body.Lemma);
        t["value"].SetString(token.Body.Value);
        t["pos"].SetIntNumber(token.Pos);
        t["length"].SetIntNumber(token.Length);
    }

    return output;
}

void TRulesMatcher::Load(const TString& input, const TGazetteer* gazetteer) {
    try {
        AcceptedRules = NTokenLogic::TMatcher::Parse(input, gazetteer);
        AcceptedRules->SetResolveGazetteerAmbiguity(false);
    } catch (const yexception& e) {
        ythrow yexception() << "Error in TokenLogic matcher: Can't compile source rule "
                            << input
                            << " (" << e.what() << ")"
                            << Endl;
    }
}

TMatchResults TRulesMatcher::Match(const TRichNodePtr& treeRoot, const TGztResults& gztres, EMatchMode mode) const {
    const TConstNodesVector wordNodes = TUserWordsConstIterator(treeRoot).Collect();

    NSorted::TSortedVector<wchar16> delimiters = {'#'};

    TVector<size_t> nodesToSymbols;
    NReMorph::NRichNode::TNodeInputSymbols symbols = NReMorph::NRichNode::CreateInputSymbols(wordNodes,
                                                                                             nodesToSymbols,
                                                                                             TLangMask(),
                                                                                             delimiters);

    NReMorph::NRichNode::TGztResultIter iter(gztres, wordNodes, nullptr, nodesToSymbols);

    NReMorph::NRichNode::TNodeInput input;
    AcceptedRules->CreateInput(input, symbols, iter);

    NTokenLogic::TTokenLogicResults acceptedResults;

    switch (mode) {
        case EMatchMode::SearchAll:
            AcceptedRules->SearchAll(input, acceptedResults);
            break;
        case EMatchMode::MatchAll:
            AcceptedRules->MatchAll(input, acceptedResults);
            break;
    }

    return FillResults(acceptedResults, input);
}

TMatchResults TRulesMatcher::FillResults(const NTokenLogic::TTokenLogicResults& tlResults,
                                         const NReMorph::NRichNode::TNodeInput& input) const {
    TMatchResults ruleResults;
    for (const auto& tlres : tlResults) {
        TMatchResult ruleRes;
        ruleRes.RuleName = tlres->RuleName;

        for (const auto& tokenNamed : tlres->NamedTokens) {
            TToken token;
            token.Type = tokenNamed.first;

            TDynBitMap ctx;
            NReMorph::NRichNode::TNodeInputSymbolPtr symbol = tlres->GetMatchedSymbol(input, tokenNamed.second, ctx);
            symbol->TraverseArticles(
                ctx,
                [&token](const TArticlePtr& a) {
                    TString value;
                    if (!a.GetField(TStringBuf("value"), value) || !value) {
                        return false;
                    }
                    token.Body.Value = value;
                    TString lemma, type;
                    if (a.GetField(TStringBuf("lemma"), lemma)) {
                        token.Body.Lemma = lemma;
                    }
                    if (a.GetField(TStringBuf("type"), type)) {
                        token.Type = type;
                    }
                    return true;
                });

            token.Text = symbol->GetText();

            token.Pos = symbol->GetSourcePos().first;
            token.Length = symbol->GetSourcePos().second - symbol->GetSourcePos().first;

            ruleRes.Tokens.push_back(std::move(token));
        }

        ruleResults.push_back(std::move(ruleRes));
    }
    return ruleResults;
}

} // namespace NMatcher

} // namespace NBASS
