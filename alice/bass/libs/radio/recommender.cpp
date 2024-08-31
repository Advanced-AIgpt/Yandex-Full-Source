#include "recommender.h"

#include <library/cpp/json/json_reader.h>
#include <library/cpp/resource/resource.h>

namespace NBASS::NRadio {

namespace {

TVector<TRadioRecommender::TWeightedRadioData> LoadWeightedRadioDatas(const NJson::TJsonValue::TArray& jsonDatas) {
    TVector<TRadioRecommender::TWeightedRadioData> datas;
    for (const auto& jsonData : jsonDatas) {
        auto data = TRadioRecommender::TWeightedRadioData{
            .RadioId = jsonData["fm_radio"].GetString(),
            .Weight = static_cast<uint32_t>(jsonData["play_count"].GetInteger())
        };
        datas.push_back(std::move(data));
    }
    return datas;
}

TRadioRecommender::TWeightedModel LoadWeightedModel(const TStringBuf filename) {
    TString rawModel = NResource::Find(filename);
    NJson::TJsonValue jsonModel;
    NJson::ReadJsonTree(rawModel, &jsonModel);

    TRadioRecommender::TWeightedModel model;
    for (const auto& [key, value] : jsonModel.GetMap()) {
        model[key] = LoadWeightedRadioDatas(value.GetArray());
    }
    return model;
}

const TRadioRecommender::TRadioData* TryFindRadio(const TStringBuf radioId, const TVector<TRadioRecommender::TRadioData>& radioDatas) {
    TStringBuf fixedRadioId;
    radioId.AfterPrefix("fm_", fixedRadioId);

    for (const TRadioRecommender::TRadioData& radioData : radioDatas) {
        if (radioData.RadioId == radioId || radioData.Title == radioId
            || radioData.RadioId == fixedRadioId || radioData.Title == fixedRadioId) {
            return &radioData;
        }
    }
    return nullptr;
}

const TRadioRecommender::TRadioData* TryFindRadioFromWeightedModel(NAlice::IRng& rng,
                                                                          const TVector<TRadioRecommender::TWeightedRadioData>& weightedRadioDatas,
                                                                          const TVector<TRadioRecommender::TRadioData>& radioDatas) {
    uint32_t totalWeight = 0;
    for (const auto& weightedRadioData : weightedRadioDatas) {
        if (!TryFindRadio(weightedRadioData.RadioId, radioDatas)) {
            continue;
        }
        totalWeight += weightedRadioData.Weight;
    }
    if (totalWeight == 0) {
        return nullptr;
    }

    uint32_t random = rng.RandomInteger(/* limit = */ totalWeight); // [0 ... (totalWeight-1)]
    uint32_t currentWeight = 0;
    for (const auto& weightedRadioData : weightedRadioDatas) {
        if (const auto* radioData = TryFindRadio(weightedRadioData.RadioId, radioDatas)) {
            currentWeight += weightedRadioData.Weight;
            if (currentWeight > random) {
                return radioData;
            }
        }
    }
    Y_UNREACHABLE();
}

} // anonymous namespace

TRadioRecommender::TRadioRecommender(NAlice::IRng& rng, const TVector<TRadioData>& availableRadioData)
    : Rng_{rng}
    , AvailableRadioDatas_{availableRadioData}
{
    Y_ENSURE(!AvailableRadioDatas_.empty());
}

TRadioRecommender& TRadioRecommender::SetDesiredRadioIds(const THashSet<TStringBuf>& desiredRadioIds) {
    DesiredRadioIds_ = &desiredRadioIds;
    return *this;
}

TRadioRecommender& TRadioRecommender::UseTrackIdWeightedModel(const TStringBuf trackId) {
    TrackId_ = trackId;
    return *this;
}

TRadioRecommender& TRadioRecommender::UseMetatagWeightedModel(const TStringBuf metatag) {
    Metatag_ = metatag;
    return *this;
}

const TRadioRecommender::TRadioData& TRadioRecommender::Recommend() const {
    const auto findRadioFromWeightedModel = [this](const TMaybe<TStringBuf>& id, const TWeightedModel& weightedModel) -> const TRadioData* {
        if (id.Defined()) {
            if (const auto iter = weightedModel.find(*id); iter != weightedModel.end()) {
                if (const auto* radioData = TryFindRadioFromWeightedModel(Rng_, iter->second, AvailableRadioDatas_)) {
                    return radioData;
                }
            }
        }
        return nullptr;
    };

    // 1st priority - weighted metatag model
    if (const auto* radioData = findRadioFromWeightedModel(TrackId_, TrackIdWeightedModel_)) {
        LOG(INFO) << "Found desired radio from " << AvailableRadioDatas_.size() << " available [trackId model]" << Endl;
        Y_STATS_INC_COUNTER("fm_radio_redirect_success");
        return *radioData;
    }

    // 2nd priority - weighted metatag model
    if (const auto* radioData = findRadioFromWeightedModel(Metatag_, MetatagWeightedModel_)) {
        LOG(INFO) << "Found desired radio from " << AvailableRadioDatas_.size() << " available [metatag model]" << Endl;
        Y_STATS_INC_COUNTER("fm_radio_redirect_success");
        return *radioData;
    }

    // 3rd priority - first available radio from the desired list
    if (DesiredRadioIds_ != nullptr) {
        const TRadioData* desiredRadioData = FindIfPtr(AvailableRadioDatas_.begin(), AvailableRadioDatas_.end(),
            [desiredRadioIds = *DesiredRadioIds_](auto& radioData) -> bool {
                return desiredRadioIds.contains(radioData.RadioId)
                        || desiredRadioIds.contains(radioData.Title);
            }
        );
        if (desiredRadioData != nullptr) {
            LOG(INFO) << "Found desired radio from " << AvailableRadioDatas_.size() << " available [hardcoded list]" << Endl;
            Y_STATS_INC_COUNTER("fm_radio_redirect_success");
            return *desiredRadioData;
        } else if (!DesiredRadioIds_->empty()) {
            Y_STATS_INC_COUNTER("fm_radio_redirect_failure");
        }
    }

    // 3th priority - random radio
    LOG(INFO) << "Didn't found desired radio from " << AvailableRadioDatas_.size() << " available, choose a random radio" << Endl;
    size_t index = Rng_.RandomInteger(AvailableRadioDatas_.size());
    return AvailableRadioDatas_.at(index);
}

const TRadioRecommender::TWeightedModel TRadioRecommender::TrackIdWeightedModel_ = LoadWeightedModel("tracks_to_fm_radio.json");
const TRadioRecommender::TWeightedModel TRadioRecommender::MetatagWeightedModel_ = LoadWeightedModel("metatags_to_fm_radio.json");

} // namespace NBASS::NRadio
