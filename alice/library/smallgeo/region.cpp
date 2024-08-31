#include "region.h"

#include <kernel/geodb/geodb.h>
#include <kernel/geodb/geodb.h>

#include <contrib/libs/pire/pire/pire.h>

#include <library/cpp/geolocation/calcer.h>
#include <library/cpp/resource/resource.h>
#include <library/cpp/scheme/scheme.h>

#include <util/generic/maybe.h>
#include <util/stream/input.h>
#include <util/stream/output.h>
#include <util/stream/zlib.h>
#include <util/string/builder.h>

namespace NAlice::NSmallGeo {
namespace {
using TStringField = std::string(NGeobase::TLinguistics::*);

const TStringField CASES[] = {&NGeobase::TLinguistics::NominativeCase,  &NGeobase::TLinguistics::GenitiveCase,
                              &NGeobase::TLinguistics::DativeCase,      &NGeobase::TLinguistics::PrepositionalCase,
                              &NGeobase::TLinguistics::Preposition,     &NGeobase::TLinguistics::LocativeCase,
                              &NGeobase::TLinguistics::DirectionalCase, &NGeobase::TLinguistics::AblativeCase,
                              &NGeobase::TLinguistics::AccusativeCase,  &NGeobase::TLinguistics::InstrumentalCase};

struct TPireFixedAnswer {
    TString Regexp;
    TFixElement FixElement;
};

} // namespace

// TRegion --------------------------------------------------------------------
TRegion::TRegion(const NGeobase::TRegion& region, const NGeobase::TLinguistics& lings)
    : Name(region.GetName())
    , Id(region.GetId())
    , ParentId(region.GetParentId())
    , Type(region.GetType())
    , Center(region.GetLatitude(), region.GetLongitude())
{
    for (const auto& c : CASES) {
        const auto& token = lings.*c;
        if (!token.empty())
            Linguistics.emplace_back(token);
    }

    const auto synonyms = region.GetSynonyms();
    Split(synonyms, ",", [this](TStringBuf synonym) { Synonyms.emplace_back(synonym); });
}

// TRegions --------------------------------------------------------------------
TRegions::TRegions(IInputStream& in)
    : Regions{LoadFromStream(in)}
{
    TMap<TId, TId> indexById;
    for (size_t index = 0, total = Regions.size(); index < total; ++index) {
        if (Regions[index].Type < NGeoDB::CITY) {
            indexById[Regions[index].Id] = index;
        }
    }

    for (const auto& region : Regions) {
        if (region.Type != NGeoDB::CITY || !region.ParentId) {
            continue;
        }
        TRegion currentRegion = region;
        auto modifiedRegionName = RemoveParentheses(region.Name);
        while (currentRegion.ParentId) {
            currentRegion = Regions[indexById[currentRegion.ParentId]];
            if (modifiedRegionName == RemoveParentheses(currentRegion.Name)) {
                RegionsWithSameNames[currentRegion.Id] = region.Id;
                break;
            }
        }
    }
}

void TRegions::SaveToStream(IOutputStream& output, TVector<TRegion>& regions) {
    TZLibCompress out(&output, ZLib::StreamType::GZip);
    TVectorSerializer<TVector<TRegion>>::Save(&out, regions);
}

TMaybe<TRegions::TId> TRegions::FindCityWithSameName(TId id) {
    if (RegionsWithSameNames.contains(id)) {
        return RegionsWithSameNames[id];
    }
    return Nothing();
}

// static
TVector<TRegion> TRegions::LoadFromStream(IInputStream& input) {
    TZLibDecompress in(&input, ZLib::StreamType::GZip);
    TVector<TRegion> regions;
    TVectorSerializer<TVector<TRegion>>::Load(&in, regions);
    return regions;
}

} // namespace NAlice::NSmallGeo
