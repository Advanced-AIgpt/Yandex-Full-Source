#pragma once

#include "utils.h"

#include <alice/megamind/protos/scenarios/request.pb.h>

#include <alice/library/json/json.h>
#include <alice/library/video_common/defs.h>
#include <alice/library/video_common/device_helpers.h>
#include <library/cpp/scheme/scheme.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/util/error.h>

#include <util/generic/algorithm.h>

using namespace NAlice::NVideoCommon;
using namespace NAlice::NScenarios;
using NAlice::TDeviceState;

namespace NVideoProtocol {

class IVideoIntent {
public:
    virtual ~IVideoIntent() = default;

    virtual const TString& GetName() const = 0;

    virtual bool IsIntentScreenDependant() const = 0;
    virtual NBASS::TResultValue MakeRunRequest(const TScenarioRunRequest& request, NSc::TValue& resultRequest) const = 0;

protected:
    virtual bool ShouldIntentHaveSlots() const = 0;

    virtual NBASS::TResultValue ConstructFormFromRequest(const TScenarioRunRequest& request, NSc::TValue& form,
                                                 TMaybe<TStringBuf> forceFormName = {}) const = 0;

    virtual void UpgradeForm(NSc::TValue& /* form */) const = 0;

    virtual NBASS::TResultValue ConstructSlotsFromSemanticFrames(const TScenarioRunRequest& request,
                                                                 TStringBuf formName,
                                                                 NSc::TArray& slots) const = 0;

    virtual NSc::TValue ConstructDataSourcesFromRequest(const TScenarioRunRequest& request) const = 0;

    virtual NBASS::TResultValue ConstructSlotsFromCallbackPayload(const TScenarioRunRequest& /*request*/,
                                                                  NSc::TArray& /*slots*/) const
    {
        return NBASS::TError{NBASS::TError::EType::VIDEOPROTOCOLERROR,
                             TString::Join(GetName(), " doesn't support callbacks")};
    };
};

template <class TIntent>
class TVideoIntent : public IVideoIntent {
public:
    explicit TVideoIntent(TStringBuf intentName)
            : IntentName(TString{intentName})
    {
    }

    const TString& GetName() const override {
        return IntentName;
    }

    bool ShouldIntentHaveSlots() const override;

    bool IsIntentScreenDependant() const override {
        return !TIntent::AllowedScreens.empty();
    }

    static bool IsIntentAvailable(const TScenarioRunRequest& request) {
        const auto& deviceState = DeviceState(request);
        NAlice::NVideoCommon::EScreenId currentScreen = CurrentScreenId(deviceState);
        return TIntent::AllowedScreens.empty() ||
               TIntent::AllowedScreens.contains(currentScreen);
    }

    NBASS::TResultValue MakeRunRequest(const TScenarioRunRequest& request, NSc::TValue& resultRequest) const override;

protected:
    NBASS::TResultValue ConstructFormFromRequest(const TScenarioRunRequest& request, NSc::TValue& form,
                                                 TMaybe<TStringBuf> forceFormName = {}) const override;
    NBASS::TResultValue ConstructSlotsFromSemanticFrames(const TScenarioRunRequest& request,
                                                                 TStringBuf formName,
                                                                 NSc::TArray& slots) const override;
    void UpgradeForm(NSc::TValue& /* form */) const override {
        LOG(INFO) << "No need in upgrading form for " << GetName() << Endl;
    }

    virtual NSc::TValue ConstructDataSourcesFromRequest(const TScenarioRunRequest& request) const override;

    virtual NBASS::TResultValue ConstructSlotsFromCallbackPayload(const TScenarioRunRequest& /*request*/,
                                                                  NSc::TArray& /*slots*/) const override
    {
        return NBASS::TError{NBASS::TError::EType::VIDEOPROTOCOLERROR,
                             TString::Join(GetName(), " doesn't support callbacks")};
    };

private:
    const TString IntentName;
};

class TSearchVideoIntent final : public TVideoIntent<TSearchVideoIntent> {
public:
    TSearchVideoIntent(TStringBuf scenarionName,
                       const TMaybe<NAlice::NVideoCommon::EProviderOverrideType>& providerOverride)
        : TVideoIntent{scenarionName}
        , ProviderOverride{providerOverride}
    {
    }

    static const TSet<EScreenId>& GetAllowedScreens() {
        return AllowedScreens;
    }

public:
    static const TSet<EScreenId> AllowedScreens;

private:
    TMaybe<NAlice::NVideoCommon::EProviderOverrideType> ProviderOverride;

    void UpgradeForm(NSc::TValue& form) const override {
        if (ProviderOverride) {
            TString value = ToString(*ProviderOverride);
            NSc::TValue slot = MakeJsonSlot(NAlice::NVideoCommon::SLOT_PROVIDER_OVERRIDE,
                                            NAlice::NVideoCommon::SLOT_PROVIDER_OVERRIDE_TYPE,
                                            /* optional= */ true,
                                            NSc::TValue(value));
            form["slots"].Push(slot);
        }

        NSc::TValue slot = MakeJsonSlot(SLOT_CALCULATE_VIDEO_FACTORS_ON_BASS,
                                        SLOT_CALCULATE_VIDEO_FACTORS_ON_BASS_TYPE,
                                        /* optional= */ true,
                                        SLOT_CALCULATE_VIDEO_FACTORS_ON_BASS_VALUE);
        form["slots"].Push(slot);
    }
};

class TVideoSelectionIntentBase : public TVideoIntent<TVideoSelectionIntentBase> {
public:
    TVideoSelectionIntentBase(TStringBuf intentType)
        : TVideoIntent(intentType)
    {
    }

    NBASS::TResultValue MakeRunRequest(const TScenarioRunRequest& request, NSc::TValue& resultRequest) const override;

    static TMaybe<TStringBuf> TryGetFormName(const TScenarioRunRequest& request) {
        if (TryGetIntentFrame(request, QUASAR_SELECT_VIDEO_FROM_GALLERY_BY_REMOTE_CONTROL)) {
            return QUASAR_SELECT_VIDEO_FROM_GALLERY_BY_REMOTE_CONTROL;
        }

        NAlice::NVideoCommon::EScreenId currentScreen = NAlice::NVideoCommon::CurrentScreenId(DeviceState(request));
        switch (currentScreen) {
            case NAlice::NVideoCommon::EScreenId::Gallery:
                [[fallthrough]];
            case NAlice::NVideoCommon::EScreenId::WebViewFilmsSearchGallery:
                [[fallthrough]];
            case NAlice::NVideoCommon::EScreenId::WebViewVideoSearchGallery:
                [[fallthrough]];
            case NAlice::NVideoCommon::EScreenId::TvExpandedCollection:
                [[fallthrough]];
            case NAlice::NVideoCommon::EScreenId::SearchResults:
                [[fallthrough]];
            case NAlice::NVideoCommon::EScreenId::WebviewVideoEntityWithCarousel:
                [[fallthrough]];
            case NAlice::NVideoCommon::EScreenId::WebviewVideoEntityRelated:
                [[fallthrough]];
            case NAlice::NVideoCommon::EScreenId::SeasonGallery:
                [[fallthrough]];
            case NAlice::NVideoCommon::EScreenId::WebviewVideoEntitySeasons:
                return NAlice::NVideoCommon::QUASAR_SELECT_VIDEO_FROM_GALLERY_BY_TEXT;
            case NAlice::NVideoCommon::EScreenId::TvGallery:
                [[fallthrough]];
            case NAlice::NVideoCommon::EScreenId::WebViewChannels:
                return NAlice::NVideoCommon::QUASAR_SELECT_CHANNEL_FROM_GALLERY_BY_TEXT;
            default:
                return Nothing();
        }
    }

public:
    static const TSet<EScreenId> AllowedScreens;

protected:
    virtual void ConstructSlotsFromSelectionResult(const NAlice::NScenarios::TScenarioRunRequest& /*request*/,
                                                   NSc::TValue& /*form*/) const
    {
    }
};

class TQuasarSelectVideoFromGalleryIntent final : public TVideoSelectionIntentBase {
public:
    TQuasarSelectVideoFromGalleryIntent()
        : TVideoSelectionIntentBase(NAlice::NVideoCommon::QUASAR_SELECT_VIDEO_FROM_GALLERY)
    {
    }

protected:
    void ConstructSlotsFromSelectionResult(const NAlice::NScenarios::TScenarioRunRequest& request,
                                           NSc::TValue& form) const override;

};

class TQuasarSelectVideoFromGalleryCallback final : public TVideoSelectionIntentBase {
public:
    TQuasarSelectVideoFromGalleryCallback()
        : TVideoSelectionIntentBase(NAlice::NVideoCommon::QUASAR_SELECT_VIDEO_FROM_GALLERY_CALLBACK)
    {
    }

protected:
    NBASS::TResultValue ConstructSlotsFromCallbackPayload(const TScenarioRunRequest& request,
                                                          NSc::TArray& slots) const override;
};

class TOpenCurrentVideoIntentBase : public TVideoIntent<TOpenCurrentVideoIntentBase> {
public:
    TOpenCurrentVideoIntentBase(TStringBuf intentName)
        : TVideoIntent(intentName)
    {
    }

public:
    static const TSet<EScreenId> AllowedScreens;
};

class TOpenCurrentVideoIntent final : public TOpenCurrentVideoIntentBase {
public:
    TOpenCurrentVideoIntent()
        : TOpenCurrentVideoIntentBase(NAlice::NVideoCommon::QUASAR_OPEN_CURRENT_VIDEO)
    {
    }
};

class TOpenCurrentVideoCallback final : public TOpenCurrentVideoIntentBase {
public:
    TOpenCurrentVideoCallback()
        : TOpenCurrentVideoIntentBase(NAlice::NVideoCommon::QUASAR_OPEN_CURRENT_VIDEO_CALLBACK)
    {
    }

protected:
    NBASS::TResultValue ConstructSlotsFromCallbackPayload(const TScenarioRunRequest& request,
                                                          NSc::TArray& slots) const override;
};

class TGoToVideoScreenIntent final : public TVideoIntent<TGoToVideoScreenIntent> {
public:
    TGoToVideoScreenIntent()
        : TVideoIntent(NAlice::NVideoCommon::QUASAR_GOTO_VIDEO_SCREEN)
    {
    }

    static bool IsIntentAvailable(const TScenarioRunRequest& request) {
        const auto* frame = TryGetIntentFrame(request, NAlice::NVideoCommon::QUASAR_GOTO_VIDEO_SCREEN);
        if (!frame) {
            return false;
        }

        const auto* slotScreen = TryGetSlotFromFrame(*frame, NAlice::NVideoCommon::SLOT_SCREEN);
        if (!slotScreen) {
            return false;
        }

        return IsMainScreen(CurrentScreenId(DeviceState(request)));
    }

public:
    static const TSet<EScreenId> AllowedScreens;
};

class TPaymentConfirmedIntentBase : public TVideoIntent<TPaymentConfirmedIntentBase> {
public:
    TPaymentConfirmedIntentBase(TStringBuf intentName)
        : TVideoIntent(intentName)
    {
    }

    static const TSet<EScreenId>& GetAllowedScreens() {
        return AllowedScreens;
    }

public:
    static const TSet<EScreenId> AllowedScreens;
};

class TPaymentConfirmedIntent final: public TPaymentConfirmedIntentBase {
public:
    TPaymentConfirmedIntent()
        : TPaymentConfirmedIntentBase(QUASAR_PAYMENT_CONFIRMED)
    {
    }
};

class TPaymentConfirmedCallback final: public TPaymentConfirmedIntentBase {
public:
    TPaymentConfirmedCallback()
        : TPaymentConfirmedIntentBase(QUASAR_PAYMENT_CONFIRMED_CALLBACK) {
    }

protected:
    NBASS::TResultValue ConstructSlotsFromCallbackPayload(const TScenarioRunRequest& /* request */,
                                                          NSc::TArray& /* slots */) const override
    {
        // personal_assistant.scenarios.quasar.payment_confirmed form has no slots
        return NBASS::ResultSuccess();
    }
};

class TAuthorizeVideoProviderIntent final : public TVideoIntent<TAuthorizeVideoProviderIntent> {
public:
    TAuthorizeVideoProviderIntent()
        : TVideoIntent(QUASAR_AUTHORIZE_PROVIDER_CONFIRMED)
    {
    }

    static const TSet<EScreenId>& GetAllowedScreens() {
        return AllowedScreens;
    }

public:
    static const TSet<EScreenId> AllowedScreens;
};

class TPlayerOnlyVideoIntent: public TVideoIntent<TPlayerOnlyVideoIntent> {
public:
    TPlayerOnlyVideoIntent(TStringBuf intentName)
        : TVideoIntent(intentName)
    {
    }

    static const TSet<EScreenId>& GetAllowedScreens() {
        return AllowedScreens;
    }

public:
    static const TSet<EScreenId> AllowedScreens;
};

class TPlayNextVideoIntent final : public TPlayerOnlyVideoIntent {
public:
    TPlayNextVideoIntent()
        : TPlayerOnlyVideoIntent(QUASAR_VIDEO_PLAY_NEXT_VIDEO)
    {
    }
};

class TPlayPreviousVideoIntent final : public TPlayerOnlyVideoIntent {
public:
    TPlayPreviousVideoIntent()
        : TPlayerOnlyVideoIntent(QUASAR_VIDEO_PLAY_PREV_VIDEO)
    {
    }
};

class TShowVideoSettingsIntent final : public TPlayerOnlyVideoIntent {
public:
    TShowVideoSettingsIntent()
        : TPlayerOnlyVideoIntent(VIDEO_COMMAND_SHOW_VIDEO_SETTINGS)
    {
    }
};

class TSkipVideoFragmentIntent final : public TPlayerOnlyVideoIntent {
public:
    TSkipVideoFragmentIntent()
        : TPlayerOnlyVideoIntent(VIDEO_COMMAND_SKIP_VIDEO_FRAGMENT)
    {
    }
};

class TChangeTrackIntent final : public TPlayerOnlyVideoIntent {
public:
    TChangeTrackIntent()
        : TPlayerOnlyVideoIntent(VIDEO_COMMAND_CHANGE_TRACK)
    {
    }
};

class TChangeTrackHardcodedIntent final : public TVideoIntent<TChangeTrackHardcodedIntent> {
public:
    TChangeTrackHardcodedIntent()
        : TVideoIntent(VIDEO_COMMAND_CHANGE_TRACK_HARDCODED)
    {
    }

public:
    static const TSet<EScreenId> AllowedScreens;
};

class TVideoHowLongIntent final : public TPlayerOnlyVideoIntent {
public:
    TVideoHowLongIntent()
        : TPlayerOnlyVideoIntent(VIDEO_COMMAND_VIDEO_HOW_LONG)
    {
    }
};

class TVideoFinishedTrackIntent final : public TVideoIntent<TVideoFinishedTrackIntent> {
public:
    TVideoFinishedTrackIntent()
        : TVideoIntent(QUASAR_VIDEO_PLAYER_FINISHED)
    {
    }

    static bool IsIntentAvailable(const TScenarioRunRequest& request) {
        const NAlice::TDeviceState& deviceState = DeviceState(request);
        const auto currentlyPlaying = NAlice::JsonFromProto(deviceState.GetVideo().GetCurrentlyPlaying());
        if (CurrentScreenId(DeviceState(request)) == NAlice::NVideoCommon::EScreenId::VideoPlayer
            && !(currentlyPlaying.Has("item") && currentlyPlaying["item"].Has("musical_track_id"))) {
            return true;
        }

        return false;
    }

public:
    static const TSet<EScreenId> AllowedScreens;
};

class TOpenCurrentTrailerIntent : public TVideoIntent<TOpenCurrentTrailerIntent> {
public:
    TOpenCurrentTrailerIntent()
        : TVideoIntent(NAlice::NVideoCommon::QUASAR_OPEN_CURRENT_TRAILER)
    {
    }

public:
    static const TSet<EScreenId> AllowedScreens;
};

std::unique_ptr<IVideoIntent> CreateIntent(TStringBuf intentType);

} // namespace NVideoProtocol
