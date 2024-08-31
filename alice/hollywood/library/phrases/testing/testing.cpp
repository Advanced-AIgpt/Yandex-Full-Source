#include "testing.h"

#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NTesting {

THashSet<TString> CheckPhrasesCorpus(const TPhrasesCorpus& corpus, const TPhraseCollection::TTagChecker& checkTag,
                                     const bool checkEmpty, const THashSet<TString>& maybeEmpty)
{
    THashSet<TString> groups;

    using TGroupChecker = std::function<void(const TPhraseGroup&)>;
    using TGroupSizeChecker = std::function<size_t(const TPhraseGroup&)>;
    using TGroupSizeCheckerByName = std::function<size_t(const TStringBuf)>;

    UNIT_ASSERT(!corpus.GetPhraseGroups().empty());
    for (const auto& grp : corpus.GetPhraseGroups()) {
        const auto& id = grp.GetId();
        UNIT_ASSERT(!id.empty());
        auto [it, inserted] = groups.emplace(id);
        UNIT_ASSERT_C(inserted, "duplicate PhraseGroup " << id);
        const TGroupChecker checkGroup = [&](const auto& grp) {
            if (&grp.GetId() != &id) {
                UNIT_ASSERT_C(grp.GetId().empty(), "NestedGroup should not have an Id in PhraseGroup " << id);
            }
            for (const auto& tag : grp.GetTags()) {
                UNIT_ASSERT_C(checkTag(tag), "unknown tag " << tag << " in PhraseGroup " << id);
            }
            for (const auto& phrase : grp.GetPhrases()) {
                UNIT_ASSERT(!phrase.GetText().empty());
                for (const auto& tag : phrase.GetTags()) {
                    UNIT_ASSERT_C(checkTag(tag), "unknown tag " << tag << " in a phrase of PhraseGroup " << id);
                }
            }
            if (grp.HasProbability()) {
                UNIT_ASSERT(grp.GetProbability() <= 1 && grp.GetProbability() >= 0);
            }
            for (const auto& nested : grp.GetNestedGroups()) {
                checkGroup(nested);
            }
        };
        checkGroup(grp);
    }
    THashSet<TString> includedGroups;
    for (const auto& grp : corpus.GetPhraseGroups()) {
        const auto& id = grp.GetId();
        const TGroupChecker checkIncludes = [&](const auto& grp) {
            for (const auto& inc : grp.GetIncludes()) {
                UNIT_ASSERT(!inc.empty());
                UNIT_ASSERT_C(groups.contains(inc), "unknown group " << inc << " referenced from Includes of PhraseGroup " << id);
                includedGroups.emplace(inc);
            }
            for (const auto& nested : grp.GetNestedGroups()) {
                checkIncludes(nested);
            }
        };
        checkIncludes(grp);
    }

    if (checkEmpty) {
        THashMap<TString, size_t> groupUntaggedPhraseCount;
        THashMap<TString, const TPhraseGroup*> groupPtrByName;
        for (const auto& grp : corpus.GetPhraseGroups()) {
            TGroupSizeChecker untaggedCount = [&](const auto& grp) {
                size_t count = 0;
                if (grp.TagsSize()) {
                    return count;
                }
                for (const auto& phrase : grp.GetPhrases()) {
                    count += phrase.GetTags().empty();
                }
                for (const auto& nested : grp.GetNestedGroups()) {
                    count += untaggedCount(nested);
                }
                return count;
            };
            groupPtrByName[grp.GetId()] = &grp;
            groupUntaggedPhraseCount[grp.GetId()] = untaggedCount(grp);
        }
        for (const auto& grp : corpus.GetPhraseGroups()) {
            if (includedGroups.contains(grp.GetId()) || maybeEmpty.contains(grp.GetId())) {
                continue;
            }
            THashSet<TString> visited;
            TGroupSizeCheckerByName untaggedCountWithIncludes = [&](TStringBuf name) {
                Y_ENSURE(groupPtrByName[name]);
                const auto& grp = *groupPtrByName[name];
                size_t count = 0;
                if (grp.TagsSize()) {
                    return count;
                }
                Y_ENSURE(grp.GetId());
                count = groupUntaggedPhraseCount[grp.GetId()];
                for (const auto& inc : grp.GetIncludes()) {
                    if (visited.emplace(inc).second) {
                        count += untaggedCountWithIncludes(inc);
                    }
                }
                for (const auto& nested : grp.GetNestedGroups()) {
                    if (nested.TagsSize()) {
                        continue;
                    }
                    for (const auto& inc : nested.GetIncludes()) {
                        if (visited.emplace(inc).second) {
                            count += untaggedCountWithIncludes(inc);
                        }
                    }
                }
                return count;
            };
            UNIT_ASSERT_C(untaggedCountWithIncludes(grp.GetId()), "phrase group \"" << grp.GetId() << "\" may produce empty phrase set");
        }
    }

    return groups;
}

} // namespace NAlice::NTesting
