#include "testing.h"

#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NTesting {

TTagConditionCollection CheckTagConditionsCorpus(const TTagConditionsCorpus& corpus) {
    THashSet<TString> tags;
    for (const auto& cond : corpus.GetConditions()) {
        UNIT_ASSERT(!cond.GetTag().empty());
        auto [it, inserted] = tags.emplace(cond.GetTag());
        UNIT_ASSERT_C(inserted, "duplicate Condition for tag " << cond.GetTag());
    }

    TTagConditionCollection tagConditionCollection{corpus};
    const auto checkTag = [&](const TStringBuf tag) {
        return tagConditionCollection.IsValidTag(tag);
    };

    for (const auto& cond : corpus.GetConditions()) {
        for (const auto& tag : cond.GetRequireTags()) {
            UNIT_ASSERT_C(checkTag(tag), "unknown tag " << tag << " in a RequireTags of " << cond.GetTag());
        }
        for (const auto& tag : cond.GetIncludeTags()) {
            UNIT_ASSERT_C(checkTag(tag), "unknown tag " << tag << " in a IncludeTags of " << cond.GetTag());
        }
    }

    return tagConditionCollection;
}

} // namespace NAlice::NTesting
