#include "phrases.h"

#include <util/generic/hash_set.h>
#include <util/generic/scope.h>
#include <util/generic/vector.h>

namespace NAlice {

namespace {

constexpr double MIN_PROBABILITY = 1e-9;

/// Scoped recording of matched tags in a vector
class TCheckTagsResult {
public:
    TCheckTagsResult(TVector<TStringBuf>& tagsMatched)
        : TagsMatched(tagsMatched)
        , TagsPriorSize(TagsMatched.size())
        , Matched(true)
    {}

    TCheckTagsResult(TCheckTagsResult&& other)
        : TagsMatched(other.TagsMatched)
        , TagsPriorSize(other.TagsPriorSize)
        , Matched(other.Matched)
    {
        other.Matched = false;
    }

    ~TCheckTagsResult() {
        SetFalse();
    }

    operator bool() const {
        return Matched;
    }

    void SetFalse() {
        if (Matched) {
            TagsMatched.resize(TagsPriorSize);
            Matched = false;
        }
    }

private:
    TVector<TStringBuf>& TagsMatched;
    const size_t TagsPriorSize;
    bool Matched;
};

class TPhraseCollectCtx {
private:
    const TPhraseCollection& Collection;
    const TPhraseCollection::TTagChecker& TagChecker;
    const TPhraseCollection::TPhraseConsumer& Consumer;
    THashSet<TStringBuf> Included;
    TVector<TStringBuf> TagsMatched;
    double Probability = 1;
    bool HaveConsumed = false;

private:
    bool MarkIncluded(TStringBuf name) {
        if (name) {
            return Included.emplace(name).second;
        }
        // nested groups (without a name) are always ok to include
        return true;
    }

    bool HavePhrases() const {
        return HaveConsumed;
    }

    void AddPhrase(const TPhraseGroup::TPhrase& phrase) {
        HaveConsumed |= Consumer(phrase, TagsMatched, Probability);
    }

    TCheckTagsResult CheckTags(const NProtoBuf::RepeatedPtrField<TString>& tags) {
        TCheckTagsResult result(TagsMatched);
        for (const auto& tag : tags) {
            if (!TagChecker(tag)) {
                result.SetFalse();
                break;
            }
            TagsMatched.emplace_back(tag);
        }
        return result;
    }

    static double GetProbability(const TPhraseGroup& group) {
        if (group.HasProbability()) {
            return group.GetProbability();
        }
        return 1;
    }

    void CollectFromGroup(const TPhraseGroup& group) {
        if (!MarkIncluded(group.GetId())) {
            return;
        }
        if (group.GetIsFallback() && HavePhrases()) {
            return;
        }
        Y_SCOPE_EXIT(this, oldProbabiliy = Probability) {
            Probability = oldProbabiliy;
        };
        Probability *= GetProbability(group);
        if (Probability < MIN_PROBABILITY) {
            return;
        }
        for (const auto& phrase : group.GetPhrases()) {
            if (const auto& checkTagsResult = CheckTags(phrase.GetTags())) {
                AddPhrase(phrase);
            }
        }
        for (const auto& inc : group.GetIncludes()) {
            TryCollectFromGroup(inc);
        }
        for (const auto& nested : group.GetNestedGroups()) {
            TryCollectFromGroup(nested);
        }
    }

    void TryCollectFromGroup(const TPhraseGroup& group) {
        if (const auto& checkTagsResult = CheckTags(group.GetTags())) {
            CollectFromGroup(group);
        }
    }

public:
    TPhraseCollectCtx(const TPhraseCollection& collection, const TPhraseCollection::TTagChecker& tagChecker,
                      const TPhraseCollection::TPhraseConsumer& consumer)
        : Collection(collection)
        , TagChecker(tagChecker)
        , Consumer(consumer)
    {}

    void TryCollectFromGroup(const TStringBuf phraseGroupId) {
        if (const auto groupPtr = Collection.FindPhraseGroup(phraseGroupId)) {
            TryCollectFromGroup(*groupPtr);
        }
    }
};

TPhraseCollection::TPhraseGroups MakePhraseGroupsMap(const TPhrasesCorpus& proto) {
    TPhraseCollection::TPhraseGroups phraseGroups;
    phraseGroups.reserve(proto.PhraseGroupsSize());
    for (const auto& phraseGroup : proto.GetPhraseGroups()) {
        if (const auto& id = phraseGroup.GetId()) {
            phraseGroups.try_emplace(id, phraseGroup);
        }
    }
    return phraseGroups;
}

} // namespace

TPhraseCollection::TPhraseCollection(const TPhrasesCorpus& groups)
    : PhraseGroups(MakePhraseGroupsMap(groups))
{}

void TPhraseCollection::FindPhrases(const TStringBuf name, const TPhraseCollection::TTagChecker& tagChecker, const TPhraseConsumer& consumer) const {
    TPhraseCollectCtx{*this, tagChecker, consumer}.TryCollectFromGroup(name);
}

} // namespace NAlice
