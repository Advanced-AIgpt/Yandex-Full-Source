#pragma once

#include <alice/megamind/library/context/context.h>

#include <alice/library/video_common/device_helpers.h>

namespace NAlice {

using EScreenId = NVideoCommon::EScreenId;

bool IsMainScreen(const IContext& ctx);
bool IsMediaPlayer(const IContext& ctx);
bool HasActivePlayerWidget(const IContext& ctx);
bool HasActiveMusicPlayerWidget(const IContext& ctx);
bool IsTvPluggedIn(const IContext& ctx);
EScreenId CurrentScreenId(const IContext& ctx);

} // namespace NAlice
