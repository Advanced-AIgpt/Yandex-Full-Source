#include "player.h"

namespace NBASS {

namespace NPlayer {

namespace {

template<typename T>
requires requires (const T& state) { state.CurrentlyPlaying(); }
bool IsAudioPlayerPaused(const T& audioPlayerState) {
    return audioPlayerState.CurrentlyPlaying().IsNull() || audioPlayerState.Player().Pause();
}

template<typename T>
requires requires (const T& state) { state.PlayerState(); }
bool IsAudioPlayerPaused(const T& audioPlayerState) {
    return audioPlayerState.PlayerState() != "Playing";
}

} // namespace

bool IsMusicPaused(const TContext& ctx) {
    return IsAudioPlayerPaused(ctx.Meta().DeviceState().Music());
}

bool IsRadioPaused(const TContext& ctx) {
    return IsAudioPlayerPaused(ctx.Meta().DeviceState().Radio());
}

bool IsBluetoothPlayerPaused(const TContext& ctx) {
    return IsAudioPlayerPaused(ctx.Meta().DeviceState().Bluetooth());
}

bool IsAudioPaused(const TContext& ctx) {
    return IsAudioPlayerPaused(ctx.Meta().DeviceState().AudioPlayer());
}

bool AreAudioPlayersPaused(const TContext& ctx) {
    return IsMusicPaused(ctx) && IsRadioPaused(ctx) && IsBluetoothPlayerPaused(ctx) && IsAudioPaused(ctx);
}

} // namespace NPlayer

} // namespace NBASS
