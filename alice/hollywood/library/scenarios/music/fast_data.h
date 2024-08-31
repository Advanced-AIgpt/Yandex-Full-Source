#pragma once

#include <alice/hollywood/library/fast_data/fast_data.h>
#include <alice/hollywood/library/phrases/phrases.h>
#include <alice/hollywood/library/scenarios/music/proto/fast_data.pb.h>
#include <alice/hollywood/library/tags/tags.h>

#include <util/generic/hash_set.h>

namespace NAlice::NHollywood::NMusic {

class TMusicFastData : public IFastData {
public:
    TMusicFastData(const TMusicFastDataProto& proto)
        : TargetingPuids(proto.GetTargetingPuids().begin(), proto.GetTargetingPuids().end())
    {}

    bool HasTargetingPuid(ui64 targetingPuid) const {
        return TargetingPuids.contains(targetingPuid);
    }

private:
    const THashSet<ui64> TargetingPuids;
};

class TStationPromoFastData : public IFastData {
public:
    TStationPromoFastData(const TStationPromoFastDataProto& proto)
        : NoPlusPuids(proto.GetNoPlusPuids().begin(), proto.GetNoPlusPuids().end())
    {}

    bool HasNoPlusPuid(ui64 noPlusPuid) const {
        return NoPlusPuids.contains(noPlusPuid);
    }

private:
    const THashSet<ui64> NoPlusPuids;
};

class TMusicShotsFastData : public IFastData {
public:
    TMusicShotsFastData(const TMusicShotsFastDataProto& proto)
        : PhraseCollection(proto.GetPhrasesCorpus())
        , TagConditionCollection(proto.GetTagConditionsCorpus())
    {}

    const TPhraseCollection& GetPhraseCollection() const {
        return PhraseCollection;
    }

    const TTagConditionCollection& GetTagConditionCollection() const {
        return TagConditionCollection;
    }

private:
    const TPhraseCollection PhraseCollection;
    const TTagConditionCollection TagConditionCollection;
};

} // namespace NAlice::NHollywood::NMusic
