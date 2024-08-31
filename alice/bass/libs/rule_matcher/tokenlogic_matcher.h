#pragma once

#include <library/cpp/scheme/scheme.h>
#include <kernel/gazetteer/gazetteer.h>
#include <kernel/qtree/richrequest/richnode_fwd.h>
#include <kernel/remorph/input/richtree/richtree.h>
#include <kernel/remorph/tokenlogic/tlmatcher.h>

namespace NBASS {

namespace NMatcher {

enum class EMatchMode {
    SearchAll,  /* Regexp can cover just part of the query, unless it has '!.' symbol. Return all variants of matching */
    MatchAll,   /* Regexp must cover the whole query (equialent to SearchAll with '!.' symbol). Return all variants of matching */
};

struct TToken {
    struct TTokenBody {
        TString Value;
        TString Lemma;
    };

    /** A user specified token type */
    TString Type;
    /** The word(s) of the token.
     * @see Pos, Length
     */
    TUtf16String Text;
    /** Separate values of body fields */
    TTokenBody Body;
    /** Position of the token */
    size_t Pos;
    /** The amount of words this token contains */
    size_t Length;
};

struct TMatchResult {
    NSc::TValue ConvertToJson() const;

    /** Name of the rule */
    TString RuleName;
    /** List of tokens accepted by the rule */
    TVector<TToken> Tokens;
};

using TMatchResults = TVector<TMatchResult>;

class TRulesMatcher {
public:
    void Load(const TString& input, const TGazetteer* gazetteer);
    TMatchResults Match(const TRichNodePtr& treeRoot, const TGztResults& gztres, EMatchMode mode) const;

private:
    TMatchResults FillResults(const NTokenLogic::TTokenLogicResults& tlResults, const NReMorph::NRichNode::TNodeInput& input) const;

private:
    TIntrusivePtr<NTokenLogic::TMatcher> AcceptedRules;
};

} // namespace NMatcher

} // namespace NBASS
