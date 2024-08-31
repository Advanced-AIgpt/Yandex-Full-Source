#pragma once

#include "token_id.h"
#include <alice/nlu/granet/lib/utils/flag_utils.h>
#include <alice/nlu/granet/lib/utils/packer_utils.h>
#include <library/cpp/containers/comptrie/comptrie.h>
#include <library/cpp/dbg_output/dump.h>

namespace NGranet {

// ~~~~ TRuleIndexes ~~~~

// Grammar rule is a chain of id (word or element) and rule data (contains probability of rule etc).
// Rules are stored in trie.
// Class TRuleIndexes is indexes of rules with same chain within one TGrammarElement.
struct TRuleIndexes {
    // Index of first (and most probable) rule.
    ui32 RuleIndex = 0;
    // Number of rules with same chain (usually 1).
    ui32 RuleCount = 0;

    ui32 GetRuleIndexAsFlag() const {
        return 1u << RuleIndex;
    }
};

// ~~~~ Trie of rules ~~~~

using TRuleTrie = TCompactTrie<TTokenId, TRuleIndexes>;

} // namespace NGranet

Y_DECLARE_PODTYPE(NGranet::TRuleIndexes);
DEFINE_PACKER(NGranet::TRuleIndexes, RuleIndex, RuleCount);
DEFINE_DUMPER(NGranet::TRuleIndexes, RuleIndex, RuleCount);
