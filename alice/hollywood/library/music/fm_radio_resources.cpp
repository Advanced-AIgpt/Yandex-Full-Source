#include "fm_radio_resources.h"

#include <library/cpp/resource/resource.h>
#include <util/generic/algorithm.h>
#include <util/stream/file.h>

#include <filesystem>

namespace NAlice::NHollywood::NMusic {

TFmRadioResources::TFmRadioResources(
    const NJson::TJsonValue& radioStationJson,
    const NJson::TJsonValue& radioStationsJson)
{
    TVector<T2DFMRadioPoint> radioStationTree;
    auto itEnd = radioStationJson.GetArray().cend();
    for (auto it = radioStationJson.GetArray().cbegin(); it != itEnd; ++it) {
        const auto& region = *it;
        THashMap<TString, TString> nameFreqMaps;
        THashMap<TString, TString> freqNameMaps;
        TVector<std::pair<float, TString>> sortedFreqs;
        for (auto& v : region["radiostations"].GetArray()) {
            TString name = v["name"].GetString();
            TString frequency = v["frequency"].GetStringRobust();
            float frequencyVal;
            try {
                frequencyVal = std::stof(frequency);
            } catch(const std::exception &exc) {
                Cerr << "Invalid frequency: " << frequency << ". Catch exception: " << exc.what() << Endl;
                continue;
            }
            nameFreqMaps.emplace(name, frequency);
            freqNameMaps.emplace(frequency, name);
            sortedFreqs.push_back(std::make_pair(frequencyVal, name));
        }

        i32 regionId = region["geoId"].GetInteger();
        FmRadioByName.insert({regionId, nameFreqMaps});
        FmRadioByFreq.insert({regionId, freqNameMaps});
        Sort(sortedFreqs);
        THashMap<TString, TPrevNextStations> stationNeighbors;
        for (size_t i = 0; i < sortedFreqs.size(); ++i) {
            TPrevNextStations prevNextStations;
            const TString& currStation = sortedFreqs[i].second;
            prevNextStations.Prev = (i > 0) ? sortedFreqs[i - 1].second : sortedFreqs.back().second;
            prevNextStations.Next = (i + 1 < sortedFreqs.size()) ? sortedFreqs[i + 1].second : sortedFreqs.front().second;
            stationNeighbors.emplace(currStation, prevNextStations);
        }
        FmRadioPrevNext.insert({regionId, stationNeighbors});
        radioStationTree.push_back({region["lat"].GetDouble(), region["lon"].GetDouble(), regionId});
    }

    FmRadioRegionTree = MakeHolder<NSmallGeo::T2DTree<T2DFMRadioPoint>>(radioStationTree);

    for (const auto& station : radioStationsJson.GetMap()) {
        const auto& name = station.first;
        const auto& id = station.second.GetString();
        NameToFmRadioId.emplace(name, id);
    }
}

// https://a.yandex-team.ru/arc/trunk/arcadia/alice/bass/libs/radio/fmdb.cpp?rev=r8680832#L63
i32 TFmRadioResources::GetNearest(double lat, double lon) const {
    return FmRadioRegionTree->GetNearest(lat, lon)->GetGeoId();
}

// https://a.yandex-team.ru/arc/trunk/arcadia/alice/bass/libs/radio/fmdb.cpp?rev=r8680832#L67
bool TFmRadioResources::HasRegion(i32 regionId) const {
    return FmRadioByName.contains(regionId) && FmRadioByFreq.contains(regionId);
}

// https://a.yandex-team.ru/arc/trunk/arcadia/alice/bass/libs/radio/fmdb.cpp?rev=r8680832#L71
bool TFmRadioResources::HasFmRadioByRegion(i32 regionId, const TString& radio) const {
    return FmRadioByName.at(regionId).contains(radio) || FmRadioByFreq.at(regionId).contains(radio);
}

// https://a.yandex-team.ru/arc/trunk/arcadia/alice/bass/libs/radio/fmdb.cpp?rev=r8680832#L75
TString TFmRadioResources::GetFmRadioByRegion(i32 regionId, const TString& radio) const {
    if (FmRadioByName.at(regionId).contains(radio)) {
        return FmRadioByName.at(regionId).at(radio);
    }
    return FmRadioByFreq.at(regionId).at(radio);
}

// https://a.yandex-team.ru/arc/trunk/arcadia/alice/bass/libs/radio/fmdb.cpp?rev=r8680832#L82
TMaybe<TString> TFmRadioResources::SeekFmRadioByRegion(i32 regionId, const TString& radio, bool forward) const {
    const auto* radioPrevNextByRegion = FmRadioPrevNext.FindPtr(regionId);
    if (!radioPrevNextByRegion) {
        return Nothing();
    }
    const auto* successor = radioPrevNextByRegion->FindPtr(radio);
    if (!successor) {
        return Nothing();
    }
    return forward ? successor->Next : successor->Prev;
}

// https://a.yandex-team.ru/arc/trunk/arcadia/alice/bass/libs/radio/fmdb.cpp?rev=r8680832#L92
size_t TFmRadioResources::GetNumStations(i32 regionId) const {
    const auto * prevNexts = FmRadioPrevNext.FindPtr(regionId);
    return prevNexts ? prevNexts->size() : 0;
}

} // namespace NAlice::NHollywood::NMusic
