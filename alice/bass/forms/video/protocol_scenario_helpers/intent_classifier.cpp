#include "intent_classifier.h"
#include "intents.h"
#include "utils.h"

#include <alice/bass/libs/logging_v2/bass_logadapter.h>
#include <alice/bass/libs/video_common/item_selector.h>

#include <alice/library/video_common/defs.h>
#include <alice/library/video_common/device_helpers.h>
#include <alice/library/video_common/video_provider.h>

#include <alice/library/client/client_features.h>

#include <util/string/cast.h>

using namespace NAlice;
using namespace NAlice::NVideoCommon;
using namespace NAlice::NScenarios;
using namespace NBASS::NVideoCommon;

namespace NVideoProtocol {

TStringBuf ChooseProviderForSearch(const TScenarioRunRequest& request) {
    const auto* searchVideoFrame = TryGetIntentFrame(request, SEARCH_VIDEO);
    if (searchVideoFrame) {
        TStringBuf intentType = SEARCH_VIDEO;
        const auto* slotVideoFree = TryGetSlotFromFrame(*searchVideoFrame, SLOT_FREE);
        if (slotVideoFree) {
            intentType = SEARCH_VIDEO_FREE;
        }

        const auto* slot = TryGetSlotFromFrame(*searchVideoFrame, SLOT_PROVIDER);
        if (slot && slot->GetType() == SLOT_PROVIDER_TYPE) {
            const TString& provider = slot->GetValue();
            if (IsInternetVideoProvider(provider)) {
                intentType = SEARCH_VIDEO_FREE;
            }

            if (provider == PROVIDER_KINOPOISK) {
                intentType = SEARCH_VIDEO_PAID;
            }
            // Conditions for SEARCH_VIDEO_ENTITY are unknown now
        }

        if (TSearchVideoIntent::IsIntentAvailable(request)) {
            return intentType;
        }
    }
    LOG(DEBUG) << "No slots for video search" << Endl;
    return NO_INTENT;
}

float GetItemSelectorThreshold(const NAlice::TClientFeatures& clientFeatures) {
    TMaybe<float> threshold;
    TStringBuf prefix = FLAG_PREFIX_ITEM_SELECTOR_THRESHOLD;
    clientFeatures.Experiments().OnEachFlag([&threshold, &prefix](TStringBuf flag) {
        if (flag.AfterPrefix(flag, prefix)) {
            float fvalue;
            if (TryFromString(flag, fvalue)) {
                threshold = fvalue;
            }
        }
    });

    // Short item names with scores like 0.75 ("3 кота") must be filtered.
    return 0.76;
}

bool IsNotChooseVideoAction(const TScenarioRunRequest& request) {
    const auto* scenarioFrame = TryGetIntentFrame(request, SEARCH_VIDEO);
    if (!scenarioFrame) {
        return false;
    }

    const auto* slotAction = TryGetSlotFromFrame(*scenarioFrame, SLOT_ACTION);
    if (!slotAction) {
        return false;
    }

    if (slotAction->GetType() != SLOT_ACTION_TYPE) {
        return false;
    }

    EVideoAction videoAction;
    if (!TryFromString<EVideoAction>(slotAction->GetValue(), videoAction)) {
        return false;
    }

    return videoAction == EVideoAction::Recommend ||
           videoAction == EVideoAction::Find ||
           videoAction == EVideoAction::Continue;
}

// Temporary classification until granet form is done
bool IsSelectIntentRecognized(const TScenarioRunRequest& request, NAlice::TClientFeatures clientFeatures) {
    auto selectFormName = TQuasarSelectVideoFromGalleryIntent::TryGetFormName(request);
    if (!selectFormName) {
        return false;
    }
    const auto* scenarioFrame = TryGetIntentFrame(request, *selectFormName);
    if (!scenarioFrame) {
        return false;
    }
    if (selectFormName == QUASAR_SELECT_VIDEO_FROM_GALLERY_BY_REMOTE_CONTROL) { // typed semantic frame in request
        return true;
    }

    if (IsNotChooseVideoAction(request)) {
        return false;
    }

    if (IsBegemotItemSelectorEnabled(request)) {
        return GetBegemotItemSelectorResult(request).Index != -1;
    }

    const auto* slotVideoText = TryGetSlotFromFrame(*scenarioFrame, SLOT_VIDEO_TEXT);
    if (!slotVideoText) {
        return false;
    }

    const TString& videoText = slotVideoText->GetValue();
    const TItemSelectionResult selected = SelectVideoFromGallery(request.GetBaseRequest().GetDeviceState(),
                                                                 videoText,
                                                                 NBASS::TBassLogAdapter{});
    if (selected.Index == -1 || selected.Confidence < GetItemSelectorThreshold(clientFeatures)) {
        return false;
    }

    return true;
}

bool IsSelectIntentRecognizedAndAvailable(const TScenarioRunRequest& request,
                                          NAlice::TClientFeatures clientFeatures) {
    return IsSelectIntentRecognized(request, clientFeatures) &&
        TQuasarSelectVideoFromGalleryIntent::IsIntentAvailable(request);
}

TStringBuf ChooseOpenVsSelectOnSeasonGallery(const TScenarioRunRequest& request,
                                             const bool isSelectIntentRecognizedAndAvailable) {
    const auto* openCurrentVideoFrame = TryGetIntentFrame(request, QUASAR_OPEN_CURRENT_VIDEO);
    if (openCurrentVideoFrame &&
        TOpenCurrentVideoIntent::IsIntentAvailable(request))
    {
        if (isSelectIntentRecognizedAndAvailable) {
            const auto* slotSeason = TryGetSlotFromFrame(*openCurrentVideoFrame, SLOT_SEASON);
            const auto* slotEpisode = TryGetSlotFromFrame(*openCurrentVideoFrame, SLOT_EPISODE);
            if (slotEpisode || slotSeason) {
                return QUASAR_OPEN_CURRENT_VIDEO;
            }
            return QUASAR_SELECT_VIDEO_FROM_GALLERY;
        }
        return QUASAR_OPEN_CURRENT_VIDEO;
    }
    if (isSelectIntentRecognizedAndAvailable) {
        return QUASAR_SELECT_VIDEO_FROM_GALLERY;
    }
    return ChooseProviderForSearch(request);
}

TStringBuf ChooseIntent(const TScenarioRunRequest& request, NAlice::TClientFeatures clientFeatures) {
    const auto& deviceState = DeviceState(request);
    const auto currentScreen = CurrentScreenId(deviceState);
    const bool isSelectIntentRecognizedAndAvailable =
        IsSelectIntentRecognizedAndAvailable(request, clientFeatures);

    LOG(DEBUG) << "Choosing intent on " << currentScreen << " screen" << Endl;
    switch (currentScreen) {
        case EScreenId::MordoviaMain:
            [[fallthrough]];
        case EScreenId::Main:
            [[fallthrough]];
        case EScreenId::TvMain: {
            if (HasIntentFrame(request, QUASAR_GOTO_VIDEO_SCREEN) &&
                TGoToVideoScreenIntent::IsIntentAvailable(request))
            {
                return QUASAR_GOTO_VIDEO_SCREEN;
            }
            break;
        }
        case EScreenId::Gallery:
            [[fallthrough]];
        case EScreenId::WebViewFilmsSearchGallery:
            [[fallthrough]];
        case EScreenId::WebViewVideoSearchGallery:
            [[fallthrough]];
        case EScreenId::TvExpandedCollection:
            [[fallthrough]];
        case EScreenId::SearchResults: {
            if (isSelectIntentRecognizedAndAvailable) {
                return QUASAR_SELECT_VIDEO_FROM_GALLERY;
            }
            break;
        }
        case EScreenId::TvGallery:
            [[fallthrough]];
        case EScreenId::WebViewChannels: {
            if (isSelectIntentRecognizedAndAvailable) {
                return QUASAR_SELECT_VIDEO_FROM_GALLERY;
            }
            break;
        }
        case EScreenId::WebviewVideoEntitySeasons:
            [[fallthrough]];
        case EScreenId::SeasonGallery: {
            if (HasIntentFrame(request, QUASAR_PAYMENT_CONFIRMED) &&
                TPaymentConfirmedIntent::IsIntentAvailable(request))
            {
                return QUASAR_PAYMENT_CONFIRMED;
            }
            return ChooseOpenVsSelectOnSeasonGallery(request, isSelectIntentRecognizedAndAvailable);
        }
        case EScreenId::WebviewVideoEntityWithCarousel:
            [[fallthrough]];
        case EScreenId::WebviewVideoEntityRelated:
            if (isSelectIntentRecognizedAndAvailable) {
                return QUASAR_SELECT_VIDEO_FROM_GALLERY;
            }
            [[fallthrough]];
        case EScreenId::Description:
            [[fallthrough]];
        case EScreenId::ContentDetails:
            [[fallthrough]];
        case EScreenId::WebViewVideoEntity:
            [[fallthrough]];
        case EScreenId::WebviewVideoEntityDescription: {
            if (HasIntentFrame(request, QUASAR_OPEN_CURRENT_TRAILER) &&
                TOpenCurrentTrailerIntent::IsIntentAvailable(request))
            {
                return QUASAR_OPEN_CURRENT_TRAILER;
            }
            if (HasIntentFrame(request, QUASAR_OPEN_CURRENT_VIDEO) &&
                TOpenCurrentVideoIntent::IsIntentAvailable(request))
            {
                return QUASAR_OPEN_CURRENT_VIDEO;
            }
            if (HasIntentFrame(request, QUASAR_AUTHORIZE_PROVIDER_CONFIRMED) &&
                TAuthorizeVideoProviderIntent::IsIntentAvailable(request))
            {
                return QUASAR_AUTHORIZE_PROVIDER_CONFIRMED;
            }
            if (HasIntentFrame(request, QUASAR_PAYMENT_CONFIRMED) &&
                TPaymentConfirmedIntent::IsIntentAvailable(request))
            {
                return QUASAR_PAYMENT_CONFIRMED;
            }
            break;
        }
        case EScreenId::Payment:
            if (HasIntentFrame(request, QUASAR_OPEN_CURRENT_VIDEO) &&
                TOpenCurrentVideoIntent::IsIntentAvailable(request))
            {
                return QUASAR_OPEN_CURRENT_VIDEO;
            }
            [[fallthrough]];
        case EScreenId::VideoPlayer: {
            if (HasIntentFrame(request, QUASAR_OPEN_CURRENT_VIDEO) &&
                TOpenCurrentVideoIntent::IsIntentAvailable(request))
            {
                return QUASAR_OPEN_CURRENT_VIDEO;
            }
            if (HasIntentFrame(request, WIZARD_QUASAR_VIDEO_PLAY_NEXT_VIDEO) &&
                TPlayNextVideoIntent::IsIntentAvailable(request))
            {
                return QUASAR_VIDEO_PLAY_NEXT_VIDEO;
            }
            if (HasIntentFrame(request, WIZARD_QUASAR_VIDEO_PLAY_PREV_VIDEO) &&
                TPlayPreviousVideoIntent::IsIntentAvailable(request))
            {
                return QUASAR_VIDEO_PLAY_PREV_VIDEO;
            }
            if (HasIntentFrame(request, VIDEO_COMMAND_SHOW_VIDEO_SETTINGS) &&
                TShowVideoSettingsIntent::IsIntentAvailable(request))
            {
                return VIDEO_COMMAND_SHOW_VIDEO_SETTINGS;
            }
            if (HasIntentFrame(request, VIDEO_COMMAND_SKIP_VIDEO_FRAGMENT) &&
                TSkipVideoFragmentIntent::IsIntentAvailable(request))
            {
                return VIDEO_COMMAND_SKIP_VIDEO_FRAGMENT;
            }
            if (HasIntentFrame(request, VIDEO_COMMAND_CHANGE_TRACK) &&
                TChangeTrackIntent::IsIntentAvailable(request))
            {
                return VIDEO_COMMAND_CHANGE_TRACK;
            }
            if (HasIntentFrame(request, VIDEO_COMMAND_VIDEO_HOW_LONG) &&
                TVideoHowLongIntent::IsIntentAvailable(request))
            {
                return VIDEO_COMMAND_VIDEO_HOW_LONG;
            }
            if (HasIntentFrame(request, QUASAR_VIDEO_PLAYER_FINISHED) &&
                TVideoFinishedTrackIntent::IsIntentAvailable(request))
            {
                return QUASAR_VIDEO_PLAYER_FINISHED;
            }

            [[fallthrough]];
        }
        default:
            if (HasIntentFrame(request, VIDEO_COMMAND_CHANGE_TRACK_HARDCODED)) {
                return VIDEO_COMMAND_CHANGE_TRACK_HARDCODED;
            }
            return ChooseProviderForSearch(request);
    }

    if (HasIntentFrame(request, VIDEO_COMMAND_CHANGE_TRACK_HARDCODED)) {
        return VIDEO_COMMAND_CHANGE_TRACK_HARDCODED;
    }
    if (HasIntentFrame(request, VIDEO_COMMAND_SHOW_VIDEO_SETTINGS) && clientFeatures.IsLegatus()) {
        return VIDEO_COMMAND_SHOW_VIDEO_SETTINGS;
    }
    if (HasIntentFrame(request, VIDEO_COMMAND_SKIP_VIDEO_FRAGMENT) && clientFeatures.IsLegatus()) {
        return VIDEO_COMMAND_SKIP_VIDEO_FRAGMENT;
    }
    if (HasIntentFrame(request, VIDEO_COMMAND_VIDEO_HOW_LONG) && clientFeatures.IsLegatus()) {
        return VIDEO_COMMAND_VIDEO_HOW_LONG;
    }

    return ChooseProviderForSearch(request);
}

} // NVideoProtocol
