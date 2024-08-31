#pragma once

#include <alice/library/metrics/sensors.h>
#include <alice/library/json/json.h>
#include <alice/library/logger/logger.h>

namespace NAlice::NHollywood {

namespace NImpl {

const TString EMPTY{TStringBuf("empty")};
const TString ENTITY{TStringBuf("entity")};
const TString PLAYLIST_OF_THE_DAY{TStringBuf("playlist_of_the_day")};
const TString SPECIAL_PLAYLIST{TStringBuf("special_playlist")};

TString ExtractMusicType(const NJson::TJsonValue& form);

} // namespace NImpl

void ProcessBassResponseUpdateSensors(TRTLogger& logger,
                                      NMetrics::ISensors& sensors,
                                      const NJson::TJsonValue& bassResponse,
                                      const TString& scenarioName,
                                      const TString& requestType);

} // namespace NAlice::NHollywood
