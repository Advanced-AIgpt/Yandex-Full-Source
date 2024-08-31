#include "fmdb.h"

#include <alice/bass/libs/logging_v2/logger.h>

#include <library/cpp/resource/resource.h>

#include <util/generic/algorithm.h>
#include <util/generic/maybe.h>

#include <string>

namespace NBASS {
namespace NAutomotive {

TFMRadioDatabase::TFMRadioDatabase() {
    TString rawData;
    if (!NResource::FindExact("radio_station", &rawData)) {
        ythrow yexception() << "Unable to load radio_station data";
    }
    TVector<T2DFMRadioPoint> radioTree;
    const NSc::TValue radioJSON = NSc::TValue::FromJson(rawData);

    auto itEnd = radioJSON.GetArray().cend();
    for (auto it = radioJSON.GetArray().cbegin(); it != itEnd; ++it) {
        const NSc::TValue& region = *it;
        THashMap<TString, TString> nameFreqMaps;
        THashMap<TString, TString> freqNameMaps;
        TVector<std::pair<float, TString>> sortedFreqs;
        for (auto& v: region["radiostations"].GetArray()) {
            TString name = v["name"].ForceString();
            TString frequency = v["frequency"].ForceString();
            float frequencyVal;
            try {
                frequencyVal = std::stof(frequency);
            } catch(const std::exception &exc) {
                LOG(ERR) << "Invalid frequency: " << frequency << ". Catch exception: " << exc.what() << Endl;
                continue;
            }
            nameFreqMaps.emplace(name, frequency);
            freqNameMaps.emplace(frequency, name);
            sortedFreqs.push_back(std::make_pair(frequencyVal, name));
        }

        i32 regionId = region["geoId"].GetNumber();
        RadioByName.insert({regionId, nameFreqMaps});
        RadioByFreq.insert({regionId, freqNameMaps});
        Sort(sortedFreqs);
        THashMap<TString, TPrevNextStations> stationNeighbors;
        for (size_t i = 0; i < sortedFreqs.size(); ++i) {
            TPrevNextStations prevNextStations;
            const TString& currStation = sortedFreqs[i].second;
            prevNextStations.Prev = (i > 0) ? sortedFreqs[i - 1].second : sortedFreqs.back().second;
            prevNextStations.Next = (i + 1 < sortedFreqs.size()) ? sortedFreqs[i + 1].second : sortedFreqs.front().second;
            stationNeighbors.emplace(currStation, prevNextStations);
        }
        RadioPrevNext.insert({regionId, stationNeighbors});
        radioTree.push_back({region["lat"].GetNumber(), region["lon"].GetNumber(), regionId});
    }

    RadioRegionTree = MakeHolder<NSmallGeo::T2DTree<T2DFMRadioPoint>>(radioTree);
}

i32 TFMRadioDatabase::GetNearest(double lat, double lon) const {
    return RadioRegionTree->GetNearest(lat, lon)->GetGeoId();
}

bool TFMRadioDatabase::HasRegion(i32 regionId) const {
    return RadioByName.contains(regionId) && RadioByFreq.contains(regionId);
}

bool TFMRadioDatabase::HasRadioByRegion(i32 regionId, const TString& radio) const {
    return RadioByName.at(regionId).contains(radio) || RadioByFreq.at(regionId).contains(radio);
}

TString TFMRadioDatabase::GetRadioByRegion(i32 regionId, const TString& radio) const {
    if (RadioByName.at(regionId).contains(radio)) {
        return RadioByName.at(regionId).at(radio);
    }
    return RadioByFreq.at(regionId).at(radio);
}

TMaybe<TString> TFMRadioDatabase::SeekRadioByRegion(i32 regionId, const TString& radio, bool forward) const {
    const auto* radioPrevNextByRegion = RadioPrevNext.FindPtr(regionId);
    if (!radioPrevNextByRegion)
        return Nothing();
    const auto* successor = radioPrevNextByRegion->FindPtr(radio);
    if (!successor)
        return Nothing();
    return forward ? successor->Next : successor->Prev;
}

size_t TFMRadioDatabase::GetNumStations(i32 regionId) const {
    const auto * prevNexts = RadioPrevNext.FindPtr(regionId);
    return prevNexts ? prevNexts->size() : 0;
}

} // namespace NAutomotive
} // namespace NBASS

