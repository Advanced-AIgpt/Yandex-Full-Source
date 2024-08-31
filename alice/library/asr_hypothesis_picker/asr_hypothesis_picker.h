#pragma once

#include <alice/library/client/client_info.h>
#include <alice/library/logger/logadapter.h>

#include <util/charset/wide.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NAlice {

struct TUsefulContextForReranker {
    struct TPlayersState {
        bool MusicIsOn = false;
        bool RadioIsOn = false;
        bool TimerIsOn = false;
        bool VideoIsOn = false;
        bool AlarmIsOn = false;
    };

    struct TIoTInfo {
        TVector<TString> IoTScenariosNamesAndTriggers = {};
    };

    TMaybe<TClientInfo> ClientInfo = {};
    TMaybe<TIoTInfo> IoTInfo;
    TMaybe<TPlayersState> PlayerState = {};
};


using TAsrHypothesisWideWords = TVector<TUtf16String>;
size_t PickBestHypothesis(TVector<TAsrHypothesisWideWords> asrHypothesesWords,
                          const TUsefulContextForReranker& context = {}, TLogAdapter* logger = nullptr);


using TAsrHypothesisWords = TVector<TString>;
size_t PickBestHypothesis(const TVector<TAsrHypothesisWords>& asrHypothesesWithUtf8Words,
                          const TUsefulContextForReranker& context = {}, TLogAdapter* logger = nullptr);

}  // namespace NAlice
