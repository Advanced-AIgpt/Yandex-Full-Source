#include "common.h"

#include <alice/hollywood/library/scenarios/music/common.h>
#include <alice/library/experiments/flags.h>

#include <library/cpp/cgiparam/cgiparam.h>

namespace NAlice::NHollywoodFw::NMusic {

bool IsPlayerCommandRelevant(const TRunRequest& request) {
    const bool isThinClient = request.Flags().IsExperimentEnabled(NExperiments::EXP_HW_MUSIC_THIN_CLIENT) ||
                              request.Flags().IsExperimentEnabled(NExperiments::EXP_HW_MUSIC_THIN_CLIENT_GENERATIVE) ||
                              request.Flags().IsExperimentEnabled(NExperiments::EXP_HW_MUSIC_THIN_CLIENT_FM_RADIO);
    const bool audioPlayerIsLatest = NHollywood::NMusic::IsAudioPlayerVsMusicAndBluetoothTheLatest(request.GetRunRequest().GetBaseRequest().GetDeviceState());
    return request.Client().GetInterfaces().GetHasAudioClient() && isThinClient && audioPlayerIsLatest;
}

TString GenerateSendSongPushUri(TStringBuf trackId) {
    TCgiParameters deeplinkParams;
    deeplinkParams.InsertUnescaped("playTrack", trackId);
    deeplinkParams.InsertUnescaped("openPlayer", "true");
    deeplinkParams.InsertUnescaped("lyricsMode", "true");

    const auto deeplink = TString::Join("yandexmusic://track/", trackId, "?", deeplinkParams.Print());

    deeplinkParams.InsertUnescaped("from", "alice");
    const auto webDeeplink = TString::Join("https://music.yandex.ru/track/", trackId, "?", deeplinkParams.Print());

    TCgiParameters params;
    params.InsertUnescaped("pid", "alice");
    params.InsertUnescaped("c", "alice text");
    params.InsertUnescaped("deep_link_value", deeplink);
    params.InsertUnescaped("af_dp", deeplink);
    params.InsertUnescaped("af_web_dp", webDeeplink);

    return TString::Join("https://music.onelink.me/ktG5?", params.Print());
}

} // namespace NAlice::NHollywoodFw::NMusic
