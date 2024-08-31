#pragma once

#include <alice/hollywood/library/tags/proto/tags.pb.h>

#include <alice/library/proto_eval/proto_eval.h>

#include <util/generic/hash.h>

namespace NAlice {

class TTagConditionCollection {
public:
    using TTagConditions = THashMap<TString, TTagCondition>;
    using TPrefixTags = TVector<const TTagCondition*>;

    TTagConditionCollection(const TTagConditionsCorpus& tags);

    const TTagCondition* FindTagCondition(const TStringBuf tag) const {
        return TagConditions.FindPtr(tag);
    }

    const TTagCondition* FindPrefixTagCondition(const TStringBuf tag, TStringBuf& tagSuffix) const;

    bool IsValidTag(TStringBuf tag) const;

private:
    const TTagConditions TagConditions;
    const TPrefixTags PrefixTags;

private:
    TTagConditionCollection(std::pair<TTagConditions, TPrefixTags>);
};

class TTagEvaluator {
public:
    TTagEvaluator(const TTagConditionCollection& tags, TProtoEvaluator& evaluator);

    /// Check tag condition with "tag" or "!tag" syntax
    /// \note returns false on errors
    bool CheckTag(TStringBuf tag);

private:
    using TTagValues = THashMap<TString, bool>;

    /// Get value, throw on errors
    bool GetTagValue(TStringBuf tag);
    /// Get value using the cache, throw on errors
    bool GetTagValueWithCache(TStringBuf tag);
    /// Get value, throw on errors, accepts "tag" or "!tag" syntax
    bool GetTagValueEx(TStringBuf tag);

private:
    const TTagConditionCollection& TagConditionCollection;
    TProtoEvaluator& Evaluator;
    TTagValues TagValues;
    TTagValues TagIsUsed;
};

} // namespace NAlice
