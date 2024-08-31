#include "region.h"

#include <kernel/geodb/geodb.h>

#include <contrib/libs/pire/pire/pire.h>

#include <library/cpp/geolocation/calcer.h>
#include <library/cpp/resource/resource.h>
#include <library/cpp/scheme/scheme.h>

#include <alice/bass/libs/logging_v2/logger.h>

#include <util/generic/maybe.h>
#include <util/stream/file.h>
#include <util/stream/input.h>
#include <util/stream/output.h>
#include <util/stream/zlib.h>
#include <util/string/builder.h>

namespace {
using TStringField = std::string(NGeobase::TLinguistics::*);

TStringField CASES[] = {&NGeobase::TLinguistics::NominativeCase,  &NGeobase::TLinguistics::GenitiveCase,
                        &NGeobase::TLinguistics::DativeCase,      &NGeobase::TLinguistics::PrepositionalCase,
                        &NGeobase::TLinguistics::Preposition,     &NGeobase::TLinguistics::LocativeCase,
                        &NGeobase::TLinguistics::DirectionalCase, &NGeobase::TLinguistics::AblativeCase,
                        &NGeobase::TLinguistics::AccusativeCase,  &NGeobase::TLinguistics::InstrumentalCase};

struct TPireFixedAnswer {
    TString Regex;
    NBASS::NSmallGeo::TFixElement FixElement;
};
} // namespace

namespace NBASS {
namespace NSmallGeo {

TRegion::TRegion(const NGeobase::TRegion& region, const NGeobase::TLinguistics& lings)
    : Name(region.GetName())
    , Id(region.GetId())
    , ParentId(region.GetParentId())
    , Type(region.GetType())
    , Center(region.GetLatitude(), region.GetLongitude()) {
    for (const auto& c : CASES) {
        const auto& token = lings.*c;
        if (!token.empty())
            Linguistics.emplace_back(token);
    }

    const auto synonyms = region.GetSynonyms();
    Split(synonyms, ",", [&](TStringBuf synonym) { Synonyms.emplace_back(synonym); });
}

struct TRegions::TFixListLatLon {
    TVector<TPireFixedAnswer> PireAnswers;
    THolder<Pire::NonrelocScanner> PireScanner;
};

TRegions::~TRegions() {}

TRegions::TRegions() {
    TString data;
    if (!NResource::FindExact("regions.data", &data)) {
        LOG(ERR) << "Can't find regions resource" << Endl;
        return;
    }

    TStringInput input(data);
    Regions = LoadFromStream(input);
    TMap<TId, TId> indexById;
    for (size_t index = 0; index < Regions.size(); ++index) {
        if (Regions[index].Type < NGeoDB::CITY) {
            indexById[Regions[index].Id] = index;
        }
    }

    for (auto region: Regions) {
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

    InitFixList();

    InitFixListLatLon();
}

void TRegions::InitFixList() {
    TString fixListData;
    if (!NResource::FindExact("smallgeo_fixlist.json", &fixListData)) {
        LOG(ERR) << "Can't find fixlist resource" << Endl;
        return;
    }
    NSc::TValue fixListJson = NSc::TValue::FromJson(fixListData);
    if (fixListJson.IsNull()) {
        LOG(ERR) << "Empty or invalid fixlist" << Endl;
        return;
    }
    for (const auto& fixPair: fixListJson.GetDict()) {
        FixList[TString(fixPair.first)] = TString(fixPair.second);
    }
    return;
}

void TRegions::InitFixListLatLon() {
    TString fixListLatLonData;
    if (!NResource::FindExact("latlon_fixlist.json", &fixListLatLonData)) {
        LOG(ERR) << "Can't find latlon fixlist resource" << Endl;
        return;
    }
    NSc::TValue latLonJson = NSc::TValue::FromJson(fixListLatLonData);
    if (!latLonJson.IsArray()) {
        LOG(ERR) << "Empty or invalid latlon fixlist" << Endl;
        return;
    }

    T2DTree<TRegion> regions2DTree(Regions);

    auto scanner = MakeHolder<Pire::NonrelocScanner>();
    TFixListLatLon result{};

    for (const auto& fixElement: latLonJson.GetArray()) {
        double lat = fixElement["Lat"].GetNumber();
        double lon = fixElement["Lon"].GetNumber();
        auto nearestRegionPtr = regions2DTree.GetNearest(lat, lon);
        TFixElement fixElem(TString(fixElement["Where"]),
            TLatLon(lat, lon), TString(fixElement["Preposition"]),
            TString(fixElement["Prepcase"]), nearestRegionPtr->Id);
        TStringBuf regex = fixElement["Where"].GetString();

        TVector<wchar32> ucs4;
        Pire::Encodings::Utf8().FromLocal(regex.begin(), regex.end(), std::back_inserter(ucs4));
        TPireFixedAnswer pireFixedAnswer = {TString(regex), fixElem};
        *scanner = Pire::Scanner::Glue(*scanner,
            Pire::Lexer(ucs4.begin(), ucs4.end())
            .SetEncoding(Pire::Encodings::Utf8())
            .Parse()
            .Compile<Pire::NonrelocScanner>()
        );
        result.PireAnswers.emplace_back(std::move(pireFixedAnswer));
    }

    if (scanner->Empty()) {
        LOG(ERR) << "PireScanner is too big" << Endl;
    }

    result.PireScanner.Swap(scanner);
    FixListLatLon = MakeHolder<TFixListLatLon>(std::move(result));
}

void TRegions::SaveToStream(IOutputStream& output, TVector<TRegion>& regions) {
    TZLibCompress out(&output, ZLib::StreamType::GZip);
    TVectorSerializer<TVector<TRegion>>::Save(&out, regions);
}

void TRegions::SaveToFile(const TString& path, TVector<TRegion>& regions) {
    TFileOutput output(path);
    SaveToStream(output, regions);
}

TVector<TRegion> TRegions::LoadFromStream(IInputStream& input) {
    TZLibDecompress in(&input, ZLib::StreamType::GZip);
    TVector<TRegion> regions;
    TVectorSerializer<TVector<TRegion>>::Load(&in, regions);
    return regions;
}

TVector<TRegion> TRegions::LoadFromFile(const TString& path) {
    TFileInput input(path);
    return LoadFromStream(input);
}

TMaybe<TRegions::TId> TRegions::FindCityWithSameName(TId id) {
    if (RegionsWithSameNames.contains(id)) {
        return RegionsWithSameNames[id];
    }
    return Nothing();
}

TMaybe<TString> TRegions::FindRegionInFixList(TStringBuf name) {
    if (auto fixPair = FixList.FindPtr(name)) {
        return *fixPair;
    }
    return Nothing();
}

TMaybe<TFixElement> TRegions::FindLatLonInFixList(TStringBuf name) {
    if (!FixListLatLon->PireScanner)
        return {};
    if (auto match = Pire::Runner(*FixListLatLon->PireScanner).Begin().Run(name.data(), name.size()).End()) {
        std::pair<const size_t*, const size_t*> indexes = FixListLatLon->PireScanner->AcceptedRegexps(match.State());
        if (indexes.second - indexes.first > 1) {
            LOG(WARNING) << "Multiple regex match" << Endl;
        }
        if (indexes.second - indexes.first >= 1) {
            const TPireFixedAnswer& answer = FixListLatLon->PireAnswers[*indexes.first];
            return answer.FixElement;
        }
    }
    return Nothing();
}

} // namespace NSmallGeo
} // namespace NBASS
