#include "content_parser.h"

using NAlice::NHollywood::NMusic::EContentWarning;
using NAlice::NHollywood::NMusic::TMusicQueueWrapper;
using NAlice::NHollywood::NMusic::TQueueItem;

namespace NAlice::NHollywoodFw::NMusic::NFmRadio {

namespace {

TQueueItem ParseFmRadioObject(const NJson::TJsonValue& json) {
    TQueueItem fmRadioItem;
    fmRadioItem.SetTrackId(json["radioId"].GetStringSafe());
    fmRadioItem.MutableFmRadioInfo()->SetActive(json["active"].GetBoolean());
    fmRadioItem.MutableFmRadioInfo()->SetAvailable(json["available"].GetBoolean());
    fmRadioItem.MutableFmRadioInfo()->SetFmRadioId(fmRadioItem.GetTrackId());
    fmRadioItem.MutableFmRadioInfo()->SetFmRadioStreamUrl(json["streamUrl"].GetStringSafe());
    fmRadioItem.MutableFmRadioInfo()->SetFrequency(json["frequency"].GetString()); // may be empty
    fmRadioItem.MutableFmRadioInfo()->SetColor(json["color"].GetString());
    // TODO(sparkle): remove default value after we move to /ranked/list
    fmRadioItem.MutableFmRadioInfo()->SetScore(json["score"].GetDoubleSafe(/* defaultValue = */ 0.0));
    fmRadioItem.SetTitle(json["title"].GetStringSafe());
    fmRadioItem.SetCoverUrl(json["imageUrl"].GetStringSafe());
    fmRadioItem.SetType("fm_radio");
    fmRadioItem.SetDurationMs(INT32_MAX);
    fmRadioItem.SetContentWarning(fmRadioItem.GetTrackId() == "detskoe" ? EContentWarning::ChildSafe : EContentWarning::Unknown);
    return fmRadioItem;
}

} // namespace

TFmRadioList TFmRadioList::ParseFromJson(const NJson::TJsonValue& json) {
    TFmRadioList list;

    for (const auto& radioJson : json["result"]["radios"].GetArraySafe()) {
        list.push_back(ParseFmRadioObject(radioJson));
    }
    return list;
}

TMaybe<size_t> TFmRadioList::GetIndexById(TStringBuf radioId) {
    for (size_t i = 0; i < size(); ++i) {
        if ((*this)[i].GetFmRadioInfo().GetFmRadioId() == radioId) {
            return i;
        }
    }
    return Nothing();
}

TQueueItem* TFmRadioList::GetItemById(TStringBuf radioId) {
    if (const auto i = GetIndexById(radioId)) {
        return &(*this)[*i];
    }
    return nullptr;
}

void TFmRadioList::SortByScoreDesc() {
    StableSortBy(begin(), end(), [](const TQueueItem& fmRadioItem) { return fmRadioItem.GetFmRadioInfo().GetScore() * -1; });
}

void TFmRadioList::SortAlphabetically() {
    StableSortBy(begin(), end(), [](const TQueueItem& fmRadioItem) { return fmRadioItem.GetTitle(); });
}

EStationStatus PutFmRadioToMusicQueue(TQueueItem&& fmRadioItem, TMusicQueueWrapper& mq) {
    if (!fmRadioItem.GetFmRadioInfo().GetActive()) {
        return EStationStatus::Inactive;
    } else if (!fmRadioItem.GetFmRadioInfo().GetAvailable()) {
        return EStationStatus::Unavailable;
    }
    mq.UpdateContentId(fmRadioItem.GetTrackId());
    mq.TryAddItem(std::move(fmRadioItem), /* hasMusicSubscription = */ false);
    return EStationStatus::OK;
}

} // NAlice::NHollywoodFw::NMusic::NFmRadio
