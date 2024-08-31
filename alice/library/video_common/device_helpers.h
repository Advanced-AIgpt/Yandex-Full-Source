#pragma once

#include <alice/megamind/protos/common/device_state.pb.h>

#include <alice/library/video_common/defs.h>

namespace NAlice::NVideoCommon {

bool IsMediaPlayer(TMaybe<EScreenId> screenId);
bool IsMediaPlayer(EScreenId screenId);
bool IsDeviceStateWellFormed(const TDeviceState& deviceState);
bool IsDeviceStateWellFormed(EScreenId screenId, const TDeviceState& deviceState);

bool IsMediaPlayer(const TDeviceState& deviceState);

bool IsTvPluggedIn(const TDeviceState& deviceState);

EScreenId CurrentScreenId(const TDeviceState& deviceState);

bool IsMainScreen(TMaybe<EScreenId> screenId);
bool IsMainScreen(EScreenId screenId);
bool IsMainScreen(const TDeviceState& deviceState);

bool IsWebViewGalleryScreen(TMaybe<EScreenId> screenId);
bool IsWebViewGalleryScreen(EScreenId screenId);
bool IsWebViewGalleryScreen(const TDeviceState& deviceState);

bool IsGalleryScreen(TMaybe<EScreenId> screenId);
bool IsGalleryScreen(EScreenId screenId);
bool IsGalleryScreen(const TDeviceState& deviceState);

bool IsWebViewDescriptionScreen(TMaybe<EScreenId> screenId);
bool IsWebViewDescriptionScreen(EScreenId screenId);
bool IsWebViewDescriptionScreen(const TDeviceState& deviceState);

bool IsContentDetailsScreen(TMaybe<EScreenId> screenId);
bool IsContentDetailsScreen(EScreenId screenId);
bool IsContentDetailsScreen(const TDeviceState& deviceState);

bool IsDescriptionScreen(TMaybe<EScreenId> screenId);
bool IsDescriptionScreen(EScreenId screenId);
bool IsDescriptionScreen(const TDeviceState& deviceState);

bool IsWebViewSeasonGalleryScreen(TMaybe<EScreenId> screenId);
bool IsWebViewSeasonGalleryScreen(EScreenId screenId);
bool IsWebViewSeasonGalleryScreen(const TDeviceState& deviceState);

bool IsSeasonGalleryScreen(TMaybe<EScreenId> screenId);
bool IsSeasonGalleryScreen(EScreenId screenId);
bool IsSeasonGalleryScreen(const TDeviceState& deviceState);

bool IsWebViewChannelsScreen(TMaybe<EScreenId> screenId);
bool IsWebViewChannelsScreen(EScreenId screenId);
bool IsWebViewChannelsScreen(const TDeviceState& deviceState);

bool IsChannelsScreen(TMaybe<EScreenId> screenId);
bool IsChannelsScreen(EScreenId screenId);
bool IsChannelsScreen(const TDeviceState& deviceState);

bool IsItemSelectionAvailable(TMaybe<EScreenId> screenId);
bool IsItemSelectionAvailable(EScreenId screenId);

bool IsTvItemSelectionAvailable(const TDeviceState& deviceState);

bool IsSearchResultsScreen(TMaybe<EScreenId> screenId);
bool IsSearchResultsScreen(EScreenId screenId);
bool IsSearchResultsScreen(const TDeviceState& deviceState);

} // namespace NAlice::NVideoCommon
