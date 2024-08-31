#include "geo.h"
#include "entities_collector.h"
#include <alice/nlu/granet/lib/sample/entity_utils.h>
#include <alice/nlu/granet/lib/utils/utils.h>
#include <library/cpp/scheme/scheme.h>
#include <util/generic/hash.h>

using namespace NNlu;
using namespace NGranet;

namespace NBg::NAliceEntityCollector {

// ~~~~ Geo entities for Alice native intents ~~~~

static NSc::TValue ConvertGeoValue(const NProto::TExternalMarkupProto::TGeoAddr& address) {
    NSc::TValue value;
    value["PossibleCityId"].AppendAll(address.GetPossibleCityId());
    value["BestGeoId"] = address.GetBestGeoId();
    value["BestInheritedId"] = address.GetBestInheritedId();
    for (const NProto::TExternalMarkupProto::TGeoAddr::TField& field : address.GetFields()) {
        value[field.GetType()] = field.GetName();
    }
    return value;
}

TVector<NGranet::TEntity> CollectGeo(const NProto::TExternalMarkupProto& markup) {
    TVector<NGranet::TEntity> entities;
    TDynBitMap coverage;

    // {entity-type, entity-interval}
    using TEntityKey = std::pair<TInterval, TString>;
    THashSet<TEntityKey> entitySet;

    for (const NProto::TExternalMarkupProto::TGeoAddr& address : markup.GetGeoAddr()) {
        const TInterval interval = ToInterval(address.GetTokens());

        TDynBitMap newPositions;
        newPositions.Set(interval.Begin, interval.End);
        const bool isAlternative = coverage.HasAny(newPositions);
        coverage |= newPositions;

        const double logprob = isAlternative
            ? NEntityLogProbs::GEO_ADDR_ALTERNATIVE
            : NEntityLogProbs::GEO_ADDR;

        for (const NProto::TExternalMarkupProto::TGeoAddr::TField& field : address.GetFields()) {
            TEntity fieldEntity;
            fieldEntity.Interval = ToInterval(field.GetTokens());
            fieldEntity.Type = NEntityTypePrefixes::GEO_ADDR + field.GetType();
            fieldEntity.Value = field.GetName();
            fieldEntity.Quality = address.GetWeight();
            fieldEntity.LogProbability = logprob;
            if (TryInsert(TEntityKey{fieldEntity.Interval, fieldEntity.Type}, &entitySet)) {
                entities.emplace_back(std::move(fieldEntity));
            }
        }

        TEntity addressEntity;
        addressEntity.Interval = ToInterval(address.GetTokens());
        addressEntity.Type = NEntityTypes::GEO_ADDR_ADDRESS;
        addressEntity.Value = ConvertGeoValue(address).ToJson();
        addressEntity.Quality = address.GetWeight();
        addressEntity.LogProbability = logprob;
        if (TryInsert(TEntityKey{addressEntity.Interval, addressEntity.Type}, &entitySet)) {
            entities.emplace_back(std::move(addressEntity));
        }
    }
    return entities;
}

TVector<NGranet::TEntity> CollectMarkupEntities(const NProto::TExternalMarkupProto& markup) {
    return CollectGeo(markup);
}

// ~~~~ Geo entities for PASkills ~~~~

static const THashMap<TString, TString> WIZARD_TO_SKILLS_NER_TYPE = {
    {"Country", "country"},
    {"City", "city"},
    {"Street", "street"},
    {"HouseNumber", "house_number"},
    {"Airport", "airport"},
};

static NSc::TValue ConvertPASkillsGeoValue(const NProto::TExternalMarkupProto::TGeoAddr& address) {
    NSc::TValue value;
    for (const NProto::TExternalMarkupProto::TGeoAddr::TField& field : address.GetFields()) {
        if (const TString* key = WIZARD_TO_SKILLS_NER_TYPE.FindPtr(field.GetType())) {
            value[*key] = field.GetName();
        }
    }
    return value;
}

TVector<NGranet::TEntity> CollectPASkillsGeo(const NProto::TExternalMarkupProto& markup) {
    TVector<TEntity> entities;
    TDynBitMap coverage;

    for (const NProto::TExternalMarkupProto::TGeoAddr& address : markup.GetGeoAddr()) {
        const TInterval interval = ToInterval(address.GetTokens());

        TDynBitMap newPositions;
        newPositions.Set(interval.Begin, interval.End);
        if (coverage.HasAny(newPositions)) {
            continue;
        }
        coverage |= newPositions;

        NSc::TValue value = ConvertPASkillsGeoValue(address);
        if (value.IsNull()) {
            continue;
        }

        TEntity& entity = entities.emplace_back();
        entity.Interval = interval;
        entity.Type = NEntityTypes::PA_SKILLS_GEO;
        entity.Value = value.ToJson();
        entity.Quality = address.GetWeight();
        entity.LogProbability = NEntityLogProbs::GEO_ADDR;
    }
    return entities;
}

} // namespace NBg::NAliceEntityCollector
