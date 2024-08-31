#include "current_screen_features.h"

#include <alice/library/video_common/defs.h>
#include <alice/library/video_common/device_helpers.h>

#include <kernel/alice/device_state_factors_info/factors_gen.h>
#include <kernel/factor_storage/factor_storage.h>

using namespace NAliceDeviceState;

namespace NAlice {

void FillCurrentScreen(const TDeviceState& deviceState, const TFactorView view) {
    using EScreenId = NVideoCommon::EScreenId;
    EScreenId screenId = NVideoCommon::CurrentScreenId(deviceState);

    view[FI_IS_SCREEN_MAIN] = screenId == EScreenId::Main || screenId == EScreenId::MordoviaMain || screenId == EScreenId::TvMain;
    view[FI_IS_SCREEN_GALLERY] = NVideoCommon::IsGalleryScreen(screenId) || screenId == EScreenId::SearchResults || screenId == EScreenId::TvExpandedCollection;
    view[FI_IS_SCREEN_SEASON_GALLERY] = NVideoCommon::IsSeasonGalleryScreen(screenId);
    view[FI_IS_SCREEN_DESCRIPTION] = NVideoCommon::IsDescriptionScreen(screenId);
    view[FI_IS_SCREEN_PAYMENT] = screenId == EScreenId::Payment;
    view[FI_IS_SCREEN_MUSIC_PLAYER] = screenId == EScreenId::MusicPlayer;
    view[FI_IS_SCREEN_VIDEO_PLAYER] = screenId == EScreenId::VideoPlayer;
    view[FI_IS_RADIO_PLAYER] = screenId == EScreenId::RadioPlayer;
    view[FI_IS_CHANNELS_GALLERY] = NVideoCommon::IsChannelsScreen(screenId);
    // Еще есть непокрытый Bluetooth, но его очень мало в обучающих данных
}

} // namespace NAlice
