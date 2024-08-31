#include "tags.h"

#include <util/generic/hash_set.h>
#include <util/generic/scope.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>

namespace NAlice {

namespace {

constexpr TStringBuf TAG_SUFFIX_PARAM = "TagSuffix";

using TTagConditions = TTagConditionCollection::TTagConditions;
using TPrefixTags = TTagConditionCollection::TPrefixTags;

std::pair<TTagConditions, TPrefixTags> MakeTagConditionsMap(const TTagConditionsCorpus& proto) {
    std::pair<TTagConditions, TPrefixTags> result;
    TTagConditions& tagConditions = result.first;
    TPrefixTags& prefixTags = result.second;

    tagConditions.reserve(proto.ConditionsSize());
    for (const auto& tagCondition : proto.GetConditions()) {
        if (const auto& tag = tagCondition.GetTag()) {
            auto [it, inserted] = tagConditions.try_emplace(tag, tagCondition);
            if (inserted && tagCondition.GetIsPrefix()) {
                prefixTags.emplace_back(&it->second);
            }
        }
    }

    const auto lessByTag = [](auto a, auto b) { return a->GetTag() < b->GetTag(); };
    Sort(prefixTags, lessByTag);

    return result;
}

struct TTagEvaluateException : public yexception {};

} // namespace

TTagConditionCollection::TTagConditionCollection(const TTagConditionsCorpus& proto)
    : TTagConditionCollection(MakeTagConditionsMap(proto))
{}

TTagConditionCollection::TTagConditionCollection(std::pair<TTagConditions, TPrefixTags> tuple)
    : TagConditions(std::move(tuple.first))
    , PrefixTags(std::move(tuple.second))
{}

const TTagCondition* TTagConditionCollection::FindPrefixTagCondition(const TStringBuf tag, TStringBuf& tagSuffix) const {
    if (PrefixTags.empty()) {
        return {};
    }
    const auto byTag = [](const TTagCondition* tc) { return tc->GetTag(); };
    const auto prefixTag = UpperBoundBy(PrefixTags.begin(), PrefixTags.end(), tag, byTag)[-1];
    if (tag.AfterPrefix(prefixTag->GetTag(), tagSuffix)) {
        return prefixTag;
    }
    return {};
}

bool TTagConditionCollection::IsValidTag(TStringBuf tag) const {
    tag.AfterPrefix("!", tag);
    TStringBuf tagSuffix;
    return FindTagCondition(tag) || FindPrefixTagCondition(tag, tagSuffix);
}

TTagEvaluator::TTagEvaluator(const TTagConditionCollection& tags, TProtoEvaluator& evaluator)
    : TagConditionCollection(tags)
    , Evaluator(evaluator)
{}

bool TTagEvaluator::GetTagValue(const TStringBuf tag) {
    TStringBuf tagSuffix;
    Y_DEFER {
        if (tagSuffix) {
            Evaluator.SetParameterValue(TAG_SUFFIX_PARAM, {});
        }
    };

    const auto* tagCondition = TagConditionCollection.FindTagCondition(tag);
    if (!tagCondition) {
        tagCondition = TagConditionCollection.FindPrefixTagCondition(tag, tagSuffix);
        if (tagSuffix) {
            Evaluator.SetParameterValue(TAG_SUFFIX_PARAM, TString{tagSuffix});
        }
    }

    Y_ENSURE_EX(tagCondition, TTagEvaluateException() << "tag not found");

    bool& tagIsUsed = TagIsUsed[tag];
    Y_ENSURE_EX(!tagIsUsed, TTagEvaluateException() << "loop detected");

    tagIsUsed = true;
    Y_DEFER { tagIsUsed = false; };

    bool defaultValue = true;
    for (const auto& includeTag : tagCondition->GetIncludeTags()) {
        if (GetTagValueEx(includeTag)) {
            return true;
        }
        defaultValue = false;
    }
    for (const auto& requireTag : tagCondition->GetRequireTags()) {
        if (!GetTagValueEx(requireTag)) {
            return false;
        }
        defaultValue = true;
    }
    return tagCondition->HasCheck() ? Evaluator.Evaluate<bool>(tagCondition->GetCheck()) : defaultValue;
}

bool TTagEvaluator::GetTagValueWithCache(const TStringBuf tag) {
    if (const bool* valuePtr = TagValues.FindPtr(tag)) {
        return *valuePtr;
    }
    const bool value = GetTagValue(tag);
    TagValues.try_emplace(tag, value);
    return value;
}

bool TTagEvaluator::GetTagValueEx(TStringBuf tag) {
    const bool invert = tag.AfterPrefix("!", tag);
    return GetTagValueWithCache(tag) ^ invert;
}

bool TTagEvaluator::CheckTag(const TStringBuf tag) {
    try {
        return GetTagValueEx(tag);
    } catch (const TTagEvaluateException&) {
        return false;
    }
}

} // namespace NAlice
