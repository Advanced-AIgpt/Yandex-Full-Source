#include "news_fast_data.h"

namespace NAlice::NHollywood {

bool TNewsPostroll::IsEnabled(const TScenarioRunRequestWrapper& request) const {
    return IsEnable && (DisableFlag.is_null() || !request.HasExpFlag(DisableFlag)) ||
        !IsEnable && !EnableFlag.is_null() && request.HasExpFlag(EnableFlag);

}

TNewsFastData::TNewsFastData(const TVector<TSmi>& smiCollection) {
    for (auto& smiObj : smiCollection) {
        if (smiObj.GranetId) {
            SmiByGranetId.insert({smiObj.GranetId, smiObj});
        }
        if (smiObj.MementoId) {
            SmiByMementoId.insert({smiObj.MementoId, smiObj});
        }
    }
}

TNewsFastData::TNewsFastData(const TNewsFastDataProto& proto) {
    for (auto& smi : proto.GetMementableSmi()) {
        TSmi smiObj(smi);
        if (smiObj.GranetId) {
            SmiByGranetId.insert({smiObj.GranetId, smiObj});
        }
        if (smiObj.MementoId) {
            SmiByMementoId.insert({smiObj.MementoId, smiObj});
        }
    }
    for (auto& postrollProto : proto.GetRadioNewsPostrolls()) {
        RadioNewsPostrolls.emplace_back(postrollProto);
    }
}

const TSmi* TNewsFastData::GetSmiByGranetId(TString granetId) const {
    if (!SmiByGranetId.contains(granetId)) {
        return nullptr;
    }
    return &SmiByGranetId.at(granetId);
}

const TSmi* TNewsFastData::GetSmiByMementoId(TString mementoId) const {
    if (!SmiByMementoId.contains(mementoId)) {
        return nullptr;
    }
    return &SmiByMementoId.at(mementoId);
}

bool TNewsFastData::IsMementableSmi(TString granetId) const {
    return SmiByGranetId.contains(granetId) && SmiByGranetId.at(granetId).IsMementable;
}

bool TNewsFastData::HasSmi(TString granetId) const {
    return SmiByGranetId.contains(granetId);
}

bool TNewsFastData::HasMementoId(TString mementoId) const {
    return SmiByMementoId.contains(mementoId);
}


int TNewsFastData::GetPostrollsCount(const TScenarioRunRequestWrapper& request) const {
    int count = 0;
    for (const auto& postroll : RadioNewsPostrolls) {
        if (!postroll.IsEnabled(request)) {
            continue;
        }
        count ++;
    }
    return count;
}

const TNewsPostroll* TNewsFastData::GetRandomPostroll(const TScenarioRunRequestWrapper& request, NAlice::IRng& rng) const {
    int sum = 0;
    for (const auto& postroll : RadioNewsPostrolls) {
        if (!postroll.IsEnabled(request)) {
            continue;
        }
        sum += postroll.ProbaScore;
    }
    int randValue = rng.RandomInteger(sum);
    for (const auto& postroll : RadioNewsPostrolls) {
        if (!postroll.IsEnabled(request)) {
            continue;
        }
        randValue -= postroll.ProbaScore;
        if (randValue < 0) {
            return &postroll;
        }
    }
    return nullptr;
}


} // namespace NAlice::NHollywood
