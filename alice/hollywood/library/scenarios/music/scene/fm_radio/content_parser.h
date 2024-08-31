#pragma once

#include "fm_radio.h"

#include <alice/hollywood/library/scenarios/music/music_backend_api/music_queue/music_queue.h>

#include <library/cpp/json/json_value.h>

namespace NAlice::NHollywoodFw::NMusic::NFmRadio {

class TFmRadioList : public TVector<NHollywood::NMusic::TQueueItem> {
public:
    static TFmRadioList ParseFromJson(const NJson::TJsonValue& json);

    TMaybe<size_t> GetIndexById(TStringBuf radioId);
    NHollywood::NMusic::TQueueItem* GetItemById(TStringBuf radioId);

    void SortByScoreDesc();
    void SortAlphabetically();

private:
    TFmRadioList() = default;
};

EStationStatus PutFmRadioToMusicQueue(NHollywood::NMusic::TQueueItem&& fmRadioItem, NHollywood::NMusic::TMusicQueueWrapper& mq);

} // NAlice::NHollywoodFw::NMusic::NFmRadio
