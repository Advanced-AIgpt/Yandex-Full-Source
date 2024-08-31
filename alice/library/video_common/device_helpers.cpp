#include "device_helpers.h"

#include <alice/library/proto/proto_struct.h>
#include <alice/library/video_common/mordovia_webview_helpers.h>

#include <alice/protos/data/search_result/search_result.pb.h>

#include <library/cpp/scheme/scheme.h>

#include <util/string/cast.h>

namespace NAlice::NVideoCommon {

EScreenId CurrentWebViewScreenId(const NAlice::TDeviceState& deviceState) {
    TStringBuf webViewScreen = GetWebViewScreenName(deviceState.GetVideo().GetViewState());

    if (!webViewScreen.empty()) {
        EScreenId screenId;
        if (TryFromString<EScreenId>(webViewScreen, screenId)) {
            return screenId;
        }
    }

    return EScreenId::MordoviaMain;
}

bool IsMediaPlayer(TMaybe<EScreenId> screenId) {
    return screenId == EScreenId::MusicPlayer || screenId == EScreenId::VideoPlayer || screenId == EScreenId::RadioPlayer;
}

bool IsMediaPlayer(EScreenId screenId) {
    return screenId == EScreenId::MusicPlayer || screenId == EScreenId::VideoPlayer || screenId == EScreenId::RadioPlayer;
}

bool IsMediaPlayer(const TDeviceState& deviceState) {
    return IsMediaPlayer(CurrentScreenId(deviceState));
}

EScreenId CurrentScreenId(const TDeviceState& deviceState) {
    const TString& currentScreen = deviceState.GetVideo().GetCurrentScreen();

    if (!currentScreen.empty()) {
        EScreenId screenId;
        if (TryFromString<EScreenId>(currentScreen, screenId)) {
            if (screenId == EScreenId::MordoviaMain) { // all webview screens have 'mordovia_webview' screenId
                return CurrentWebViewScreenId(deviceState);
            }
            return screenId;
        }
    }

    // Return Main screen by default, if other is not provided in device_state.
    return EScreenId::Main;
}

bool IsDeviceStateWellFormed(EScreenId screenId, const TDeviceState& deviceState) {
    // feel free to improve these checks!
    switch (screenId) {
    case EScreenId::RadioPlayer:
        [[fallthrough]];
    case EScreenId::MusicPlayer:
        [[fallthrough]];
    case EScreenId::Bluetooth:
        [[fallthrough]];
    case EScreenId::Main:
        [[fallthrough]];
    case EScreenId::TvMain:
        return true;

    case EScreenId::Gallery:
        [[fallthrough]];
    case EScreenId::TvGallery:
        return deviceState.GetVideo().GetScreenState().RawItemsSize()  > 0;

    case EScreenId::SeasonGallery:
        return deviceState.GetVideo().GetScreenState().RawItemsSize() > 0 &&
               deviceState.GetVideo().GetScreenState().HasTvShowItem() &&
               deviceState.GetVideo().GetScreenState().HasSeason();

    case EScreenId::ContentDetails:
        return deviceState.GetVideo().HasTvInterfaceState() &&
               deviceState.GetVideo().GetTvInterfaceState().HasContentDetailsScreen() &&
               deviceState.GetVideo().GetTvInterfaceState().GetContentDetailsScreen().HasCurrentItem();
    case EScreenId::Description:
        [[fallthrough]];
    case EScreenId::Payment:
        return deviceState.GetVideo().GetScreenState().HasRawItem();

    case EScreenId::VideoPlayer:
        return deviceState.GetVideo().HasCurrentlyPlaying() &&
               deviceState.GetVideo().GetCurrentlyPlaying().HasRawItem();

    case EScreenId::WebViewVideoEntity:
        [[fallthrough]];
    case EScreenId::WebviewVideoEntityDescription:
        if (deviceState.GetVideo().HasViewState()) {
            return GetWebViewCurrentVideoItem(deviceState.GetVideo().GetViewState()).fields_size() != 0;
        }
        return false;

    case EScreenId::WebviewVideoEntityWithCarousel:
        [[fallthrough]];
    case EScreenId::WebviewVideoEntityRelated:
        if (deviceState.GetVideo().HasViewState()) {
            const auto& viewState = deviceState.GetVideo().GetViewState();
            if (GetWebViewCurrentVideoItem(viewState).fields_size() != 0) {
                const auto& galleryItems = GetWebViewGalleryItems(viewState);
                return galleryItems.values_size() > 0;
            }
        }
        return false;

    case EScreenId::WebviewVideoEntitySeasons:
        if (deviceState.GetVideo().HasViewState()) {
            const auto& viewState = deviceState.GetVideo().GetViewState();
            if (GetWebViewCurrentTvShowItem(viewState).fields_size() != 0) {
                if (GetWebViewGalleryItems(viewState).values_size() != 0) {
                    TProtoStructParser parser;
                    return parser.GetValueInt(viewState, "sections.0.season").Defined();
                }
            }
        }
        return false;

    case EScreenId::WebViewVideoSearchGallery:
        [[fallthrough]];
    case EScreenId::WebViewFilmsSearchGallery:
        [[fallthrough]];
    case EScreenId::WebViewChannels:
        [[fallthrough]];
    case EScreenId::MordoviaMain:
        if (deviceState.GetVideo().HasViewState()) {
            const auto& galleryItems = GetWebViewGalleryItems(deviceState.GetVideo().GetViewState());
            return galleryItems.values_size() > 0;
        }
        return false;

    case EScreenId::SearchResults:
        if (deviceState.GetVideo().HasTvInterfaceState()) {
            const auto& tvInterfaceState = deviceState.GetVideo().GetTvInterfaceState();
            if (tvInterfaceState.HasSearchResultsScreen()) {
                // device state for this screen is correct if there are some galleries and each gallery has some items
                for (size_t i = 0; i < tvInterfaceState.GetSearchResultsScreen().GalleriesSize(); ++i) {
                    if (tvInterfaceState.GetSearchResultsScreen().GetGalleries(i).ItemsSize() == 0) {
                        return false;
                    }
                }
                return tvInterfaceState.GetSearchResultsScreen().GalleriesSize() > 0;
            }
        }
        return false;
    case EScreenId::TvExpandedCollection:
        if (deviceState.GetVideo().HasTvInterfaceState()) {
            const auto& tvInterfaceState = deviceState.GetVideo().GetTvInterfaceState();
            if (tvInterfaceState.HasExpandedCollectionScreen()) {
                // device state for this screen is correct if there are some galleries and each gallery has some items
                for (size_t i = 0; i < tvInterfaceState.GetExpandedCollectionScreen().GalleriesSize(); ++i) {
                    if (tvInterfaceState.GetExpandedCollectionScreen().GetGalleries(i).ItemsSize() == 0) {
                        return false;
                    }
                }
                return tvInterfaceState.GetExpandedCollectionScreen().GalleriesSize() > 0;
            }
        }
        return false;
    }

    Y_UNREACHABLE();
}

bool IsDeviceStateWellFormed(const TDeviceState& deviceState) {
    EScreenId currentScreen = CurrentScreenId(deviceState);
    return IsDeviceStateWellFormed(currentScreen, deviceState);
}

bool IsTvPluggedIn(const TDeviceState& deviceState) {
    return deviceState.GetIsTvPluggedIn();
}

bool IsWebViewMainScreen(TMaybe<EScreenId> screenId) {
    return screenId == EScreenId::MordoviaMain;
}

bool IsWebViewMainScreen(EScreenId screenId) {
    return screenId == EScreenId::MordoviaMain;
}

bool IsWebViewMainScreen(const TDeviceState& deviceState) {
    return IsWebViewMainScreen(CurrentScreenId(deviceState));
}

bool IsMainScreen(TMaybe<EScreenId> screenId) {
    return screenId == EScreenId::Main || screenId == EScreenId::TvMain || IsWebViewMainScreen(screenId);
}

bool IsMainScreen(EScreenId screenId) {
    return screenId == EScreenId::Main || screenId == EScreenId::TvMain || IsWebViewMainScreen(screenId);
}

bool IsMainScreen(const TDeviceState& deviceState) {
    return IsMainScreen(CurrentScreenId(deviceState));
}

bool IsWebViewGalleryScreen(TMaybe<EScreenId> screenId) {
    return screenId == EScreenId::WebViewFilmsSearchGallery ||
           screenId == EScreenId::WebViewVideoSearchGallery ||
           screenId == EScreenId::WebviewVideoEntityWithCarousel ||
           screenId == EScreenId::WebviewVideoEntityRelated;
}

bool IsWebViewGalleryScreen(EScreenId screenId) {
    return screenId == EScreenId::WebViewFilmsSearchGallery ||
           screenId == EScreenId::WebViewVideoSearchGallery ||
           screenId == EScreenId::WebviewVideoEntityWithCarousel ||
           screenId == EScreenId::WebviewVideoEntityRelated;
}

bool IsWebViewGalleryScreen(const TDeviceState& deviceState) {
    return IsWebViewGalleryScreen(CurrentScreenId(deviceState));
}

bool IsGalleryScreen(TMaybe<EScreenId> screenId) {
    return screenId == EScreenId::Gallery || IsWebViewGalleryScreen(screenId);
}

bool IsGalleryScreen(EScreenId screenId) {
    return screenId == EScreenId::Gallery || IsWebViewGalleryScreen(screenId);
}

bool IsGalleryScreen(const TDeviceState& deviceState) {
    return IsGalleryScreen(CurrentScreenId(deviceState));
}

bool IsWebViewDescriptionScreen(TMaybe<EScreenId> screenId) {
    return screenId == EScreenId::WebViewVideoEntity ||
           screenId == EScreenId::WebviewVideoEntityWithCarousel ||
           screenId == EScreenId::WebviewVideoEntityDescription ||
           screenId == EScreenId::WebviewVideoEntityRelated;
}

bool IsWebViewDescriptionScreen(EScreenId screenId) {
    return screenId == EScreenId::WebViewVideoEntity ||
           screenId == EScreenId::WebviewVideoEntityWithCarousel ||
           screenId == EScreenId::WebviewVideoEntityDescription ||
           screenId == EScreenId::WebviewVideoEntityRelated;
}

bool IsWebViewDescriptionScreen(const TDeviceState& deviceState) {
    return IsWebViewDescriptionScreen(CurrentScreenId(deviceState));
}

bool IsContentDetailsScreen(TMaybe<EScreenId> screenId) {
    return screenId.Defined() && *screenId == EScreenId::ContentDetails;
}

bool IsContentDetailsScreen(EScreenId screenId) {
    return screenId == EScreenId::ContentDetails;
}

bool IsContentDetailsScreen(const TDeviceState& deviceState) {
    return IsContentDetailsScreen(CurrentScreenId(deviceState));
}

bool IsDescriptionScreen(TMaybe<EScreenId> screenId) {
    return (screenId.Defined() && *screenId == EScreenId::Description) ||
           IsContentDetailsScreen(screenId) || IsWebViewDescriptionScreen(screenId);
}

bool IsDescriptionScreen(EScreenId screenId) {
    return screenId == EScreenId::Description || IsContentDetailsScreen(screenId) || IsWebViewDescriptionScreen(screenId);
}

bool IsDescriptionScreen(const TDeviceState& deviceState) {
    return IsDescriptionScreen(CurrentScreenId(deviceState));
}

bool IsWebViewSeasonGalleryScreen(TMaybe<EScreenId> screenId) {
    return screenId == EScreenId::WebviewVideoEntitySeasons;
}

bool IsWebViewSeasonGalleryScreen(EScreenId screenId) {
    return screenId == EScreenId::WebviewVideoEntitySeasons;
}

bool IsWebViewSeasonGalleryScreen(const TDeviceState& deviceState) {
    return IsWebViewSeasonGalleryScreen(CurrentScreenId(deviceState));
}

bool IsSeasonGalleryScreen(TMaybe<EScreenId> screenId) {
    return screenId == EScreenId::SeasonGallery || IsWebViewSeasonGalleryScreen(screenId);
}

bool IsSeasonGalleryScreen(EScreenId screenId) {
    return screenId == EScreenId::SeasonGallery || IsWebViewSeasonGalleryScreen(screenId);
}

bool IsSeasonGalleryScreen(const TDeviceState& deviceState) {
    return IsSeasonGalleryScreen(CurrentScreenId(deviceState));
}

bool IsWebViewChannelsScreen(TMaybe<EScreenId> screenId) {
    return screenId == EScreenId::WebViewChannels;
}

bool IsWebViewChannelsScreen(EScreenId screenId) {
    return screenId == EScreenId::WebViewChannels;
}

bool IsWebViewChannelsScreen(const TDeviceState& deviceState) {
    return IsWebViewChannelsScreen(CurrentScreenId(deviceState));
}

bool IsChannelsScreen(TMaybe<EScreenId> screenId) {
    return screenId == EScreenId::TvGallery || IsWebViewChannelsScreen(screenId);
}

bool IsChannelsScreen(EScreenId screenId) {
    return screenId == EScreenId::TvGallery || IsWebViewChannelsScreen(screenId);
}

bool IsChannelsScreen(const TDeviceState& deviceState) {
    return IsChannelsScreen(CurrentScreenId(deviceState));
}

bool IsItemSelectionAvailable(TMaybe<EScreenId> screenId) {
    return IsWebViewMainScreen(screenId) || IsGalleryScreen(screenId) || screenId == EScreenId::SearchResults || screenId == EScreenId::TvExpandedCollection;
}

bool IsItemSelectionAvailable(EScreenId screenId) {
    return IsWebViewMainScreen(screenId) || IsGalleryScreen(screenId) || screenId == EScreenId::SearchResults || screenId == EScreenId::TvExpandedCollection;
}

bool IsTvItemSelectionAvailable(const TDeviceState& deviceState) {
    EScreenId screen = CurrentScreenId(deviceState);
    return screen == EScreenId::SearchResults || screen == EScreenId::TvExpandedCollection;
}

bool IsSearchResultsScreen(TMaybe<EScreenId> screenId) {
    return screenId == EScreenId::SearchResults;
}

bool IsSearchResultsScreen(EScreenId screenId) {
    return screenId == EScreenId::SearchResults;
}

bool IsSearchResultsScreen(const TDeviceState& deviceState) {
    return IsSearchResultsScreen(CurrentScreenId(deviceState));
}

} // namespace NAlice::NVideoCommon
