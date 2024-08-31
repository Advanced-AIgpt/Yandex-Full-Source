#include "query_parser.h"

#include "bow.h"
#include "defs.h"
#include "preprocessor.h"

#include <library/cpp/regex/pcre/regexp.h>

#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>


namespace NAlice::NIot {

namespace {

// Helps using << easily, when logBuilder is in scope
#define LOG(x) IOT_LOG(logBuilder, x);

class TGroupedEntities {
public:
    TGroupedEntities(const TVector<TRawEntity>& entities, int nTokens)
        : GroupedEntities_(nTokens)
        , NTokens_(nTokens)
    {
        for (const auto& entity : entities) {
            if (entity.AsEntity().GetStart() < GroupedEntities_.ysize() &&
                entity.AsEntity().GetEnd() <= GroupedEntities_.ysize())
            {
                GroupedEntities_[entity.AsEntity().GetStart()].push_back(&entity);
            }
        }
    }

    const TVector<const TRawEntity*>& EntitiesFromPosition(int pos) const {
        return GroupedEntities_[pos];
    }

    int NTokens() const {
        return NTokens_;
    }

private:
    TVector<TVector<const TRawEntity*>> GroupedEntities_;
    int NTokens_;
};

constexpr int COMPLEXITY_UPPER_BOUND = 2500;

struct TMatchResult {
    TVector<TRawEntity> Entities;
};

using TTypeToBowIndexTokens = THashMap<TString, TParsedBowTokens>;

void AddValue(const NSc::TValue& value, NSc::TValue& collection) {
    if (value.IsArray()) {
        collection.AppendAll(value.GetArray());
    } else {
        collection.Push(value);
    }
}

void AddValue(const TStringBuf type, const TString& text, const NSc::TValue& value, NSc::TValue& ph,
              TTypeToBowIndexTokens& typeToBowIndexTokens)
{
    if (type.StartsWith(BOW_PREFIX)) {
        typeToBowIndexTokens[type.substr(BOW_PREFIX.size())].push_back({value.GetString(), text});
    } else if (type.StartsWith(ARG_PREFIX)) {
        AddValue(value, ph["arg"][type.substr(ARG_PREFIX.size())]);
    } else if (type.StartsWith(FST_PREFIX)) {
        AddValue(value, ph["fst"][type.substr(FST_PREFIX.size())]);
    } else if (type.StartsWith(COMPLEX_PREFIX)) {
        for (const auto& [subType, subValue] : value.GetDict()) {
            AddValue(subType, text, subValue, ph, typeToBowIndexTokens);
        }
    } else {
        AddValue(value, ph[type]);
    }
}

void Match(const int i, const TGroupedEntities& groupedEntities, TVector<TRawEntity>& current, TVector<TMatchResult>& result) {
    if (i == groupedEntities.NTokens()) {
        result.push_back({current});
        return;
    }

    Y_ASSERT(i < groupedEntities.NTokens());
    for (const auto* entity : groupedEntities.EntitiesFromPosition(i)) {
        current.push_back(*entity);
        Match(entity->AsEntity().GetEnd(), groupedEntities, current, result);
        current.pop_back();
    }
}

TVector<TMatchResult> Match(const TGroupedEntities& groupedEntities, TStringBuilder* logBuilder) {
    int complexityUpperBound = 1;
    for (int i = 0; i < groupedEntities.NTokens(); ++i) {
        complexityUpperBound *= Max(1, groupedEntities.EntitiesFromPosition(i).ysize());
        if (complexityUpperBound > COMPLEXITY_UPPER_BOUND) {
            LOG("Exceeded complexity upper bound (" << complexityUpperBound << " > " << COMPLEXITY_UPPER_BOUND << ")");
            return {};
        }
    }

    TVector<TMatchResult> result;
    TVector<TRawEntity> buffer;
    Match(0, groupedEntities, buffer, result);
    return result;
}

TRawParsingHypotheses TryFormParsingHypotheses(const TMatchResult& matchResult, ELanguage language, TStringBuilder* logBuilder) {
    TTypeToBowIndexTokens typeToBowIndexTokens;
    for (const auto& [type, values] : GetPreprocessor(language).Entities.GetDict()) {
        typeToBowIndexTokens[type]; // initializing with empty vectors
    }

    NSc::TValue ph;
    THashSet<std::tuple<TString, TString, NSc::TValue>> seenTextTypeValueCollection;
    auto& rawEntities = ph["raw_entities"];
    for (const auto& e : matchResult.Entities) {
        rawEntities.Push(e.AsValue());

        const auto textTypeValueTuple = std::make_tuple(e.AsEntity().GetText(), e.AsEntity().GetTypeStr(), e.AsValue()["value"]);
        if (seenTextTypeValueCollection.contains(textTypeValueTuple)) {
            continue;
        }
        seenTextTypeValueCollection.insert(textTypeValueTuple);

        AddValue(e.AsEntity().GetTypeStr(), e.AsEntity().GetText(), e.AsValue()["value"], ph, typeToBowIndexTokens);
    }
    LOG("Got match result: " << rawEntities.ToJson());

    TRawParsingHypotheses results{ph};
    for (const auto& p : typeToBowIndexTokens) {
        const auto& type = p.first;
        const auto& tokens = p.second;
        auto values = GetPreprocessor(language).BOWIndex.ExtractValues(type, tokens);
        if (values.empty() && !tokens.empty()) {
            IOT_LOG_NONEWLINE(logBuilder, "No values for type " << type << " and tokens [ ");
            for (const auto& token : tokens) {
                IOT_LOG_NONEWLINE(logBuilder, "{ " << token.Form << " " << token.ExactForm << " } ");
            }
            LOG("]");
            return {};
        }

        if (values.empty()) {
            continue;
        }

        TRawParsingHypotheses updatedResults;
        for (const auto& result : results) {
            for (const auto& value : values) {
                NSc::TValue updatedResult = result;
                updatedResult[type].Push(value);
                updatedResults.push_back(updatedResult);
            }
        }
        results = updatedResults;
    }

    return results;
}

TVector<TRawEntity> FilterOutEntitiesNotForHypotheses(TVector<TRawEntity> entities) {
    EraseIf(entities, [](const TRawEntity& entity) {
        return IsIn({"multiroom_all_devices", "custom_button"}, entity.AsEntity().GetTypeStr());
    });
    return entities;
}

} // namespace

TRawParsingHypotheses Parse(const TIoTEntitiesInfo& entitiesInfo, ELanguage language, TStringBuilder* logBuilder) {
    LOG("Generating parsing hypotheses");

    const auto filteredEntities = FilterOutEntitiesNotForHypotheses(entitiesInfo.Entities);

    TRawParsingHypotheses results;
    for (const auto& matchResult : Match(TGroupedEntities(filteredEntities, entitiesInfo.NTokens), logBuilder)) {
        for (const auto& result : TryFormParsingHypotheses(matchResult, language, logBuilder)) {
            results.push_back(result);
            LOG("Added parsing hypothesis: " << result.ToJson());
        }
    }

    LOG("Finished generating parsing hypotheses");
    return results;
}


} // namespace NAlice::NIot
