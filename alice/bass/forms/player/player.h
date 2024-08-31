#pragma once

#include <alice/bass/forms/context/context.h>

namespace NBASS {

namespace NPlayer {

bool IsMusicPaused(const TContext& ctx);
bool IsRadioPaused(const TContext& ctx);
bool IsBluetoothPlayerPaused(const TContext& ctx);
bool AreAudioPlayersPaused(const TContext& ctx);

} // namespace NPlayer

} // namespace NBASS
