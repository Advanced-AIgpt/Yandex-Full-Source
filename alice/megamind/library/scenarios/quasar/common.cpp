#include "common.h"

#include <util/string/cast.h>

#include <alice/library/video_common/device_helpers.h>

#include <google/protobuf/struct.pb.h>

namespace NAlice {

namespace {

const TString CURRENTLY_PLAYING = "currently_playing";

}

bool IsMainScreen(const IContext& ctx) {
    const auto currentScreen = NVideoCommon::CurrentScreenId(ctx.SpeechKitRequest()->GetRequest().GetDeviceState());
    return currentScreen == NVideoCommon::EScreenId::Main || currentScreen == NVideoCommon::EScreenId::MordoviaMain || currentScreen == NVideoCommon::EScreenId::TvMain;
}

bool IsMediaPlayer(const IContext& ctx) {
    return NVideoCommon::IsMediaPlayer(ctx.SpeechKitRequest()->GetRequest().GetDeviceState());
}

bool HasActivePlayerWidget(const IContext& ctx) {
    const TDeviceState& deviceState = ctx.SpeechKitRequest().Proto().GetRequest().GetDeviceState();
    return (
        deviceState.HasMusic() && deviceState.GetMusic().HasCurrentlyPlaying() ||
        deviceState.HasVideo() && deviceState.GetVideo().HasCurrentlyPlaying() ||
        deviceState.HasRadio() && deviceState.GetRadio().fields().count(CURRENTLY_PLAYING) != 0
    );
}

bool HasActiveMusicPlayerWidget(const IContext& ctx) {
    const TDeviceState& deviceState = ctx.SpeechKitRequest().Proto().GetRequest().GetDeviceState();
    return deviceState.HasMusic() && deviceState.GetMusic().HasCurrentlyPlaying();
}

EScreenId CurrentScreenId(const IContext& ctx) {
    return NVideoCommon::CurrentScreenId(ctx.SpeechKitRequest()->GetRequest().GetDeviceState());
}

bool IsTvPluggedIn(const IContext& ctx) {
    return NVideoCommon::IsTvPluggedIn(ctx.SpeechKitRequest()->GetRequest().GetDeviceState());
}

} // namespace NAlice
