#include "intents.h"

#include <alice/bass/libs/logging_v2/bass_logadapter.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/video_common/item_selector.h>

#include <alice/library/frame/description.h>
#include <alice/library/json/json.h>
#include <alice/library/video_common/defs.h>
#include <alice/library/video_common/device_helpers.h>
#include <alice/library/video_common/mordovia_webview_helpers.h>

#include <alice/megamind/protos/common/data_source_type.pb.h>
#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/megamind/protos/common/required_messages/required_messages.pb.h>

#include <library/cpp/scheme/scheme.h>

#include <util/string/cast.h>

using namespace NAlice::NVideoCommon;
using namespace NBASS;
using namespace NBASS::NVideoCommon;

namespace NVideoProtocol {

namespace {

const THashSet<TStringBuf> SCENARIOS_WITH_SLOTS{SEARCH_VIDEO,
                                                QUASAR_SELECT_VIDEO_FROM_GALLERY,
                                                QUASAR_SELECT_VIDEO_FROM_GALLERY_CALLBACK,
                                                QUASAR_GOTO_VIDEO_SCREEN,
                                                QUASAR_OPEN_CURRENT_VIDEO};

const THashMap<TStringBuf, std::function<std::unique_ptr<IVideoIntent>()>> INTENT_FACTORY{
    {SEARCH_VIDEO,
        [](){ return std::make_unique<TSearchVideoIntent>(SEARCH_VIDEO, Nothing()); }},
    {SEARCH_VIDEO_ENTITY,
        [](){ return std::make_unique<TSearchVideoIntent>(SEARCH_VIDEO, EProviderOverrideType::Entity); }}, // TODO SEARCH_VIDEO_ENTITY
    {SEARCH_VIDEO_FREE,
        [](){ return std::make_unique<TSearchVideoIntent>(SEARCH_VIDEO, EProviderOverrideType::FreeOnly); }}, // TODO SEARCH_VIDEO_FREE
    {SEARCH_VIDEO_PAID,
        [](){ return std::make_unique<TSearchVideoIntent>(SEARCH_VIDEO, EProviderOverrideType::PaidOnly); }}, // TODO SEARCH_VIDEO_PAID
    {QUASAR_VIDEO_PLAY_NEXT_VIDEO,
        [](){ return std::make_unique<TPlayNextVideoIntent>(); }},
    {QUASAR_VIDEO_PLAY_PREV_VIDEO,
        [](){ return std::make_unique<TPlayPreviousVideoIntent>(); }},
    {QUASAR_SELECT_VIDEO_FROM_GALLERY,
        [](){ return std::make_unique<TQuasarSelectVideoFromGalleryIntent>(); }},
    {QUASAR_SELECT_VIDEO_FROM_GALLERY_CALLBACK,
        [](){ return std::make_unique<TQuasarSelectVideoFromGalleryCallback>(); }},
    {QUASAR_GOTO_VIDEO_SCREEN,
        [](){ return std::make_unique<TGoToVideoScreenIntent>(); }},
    {QUASAR_PAYMENT_CONFIRMED,
        [](){ return std::make_unique<TPaymentConfirmedIntent>(); }},
    {QUASAR_PAYMENT_CONFIRMED_CALLBACK,
        [](){ return std::make_unique<TPaymentConfirmedCallback>(); }},
    {QUASAR_AUTHORIZE_PROVIDER_CONFIRMED,
        [](){ return std::make_unique<TAuthorizeVideoProviderIntent>(); }},
    {QUASAR_OPEN_CURRENT_VIDEO,
        [](){ return std::make_unique<TOpenCurrentVideoIntent>(); }},
    {QUASAR_OPEN_CURRENT_VIDEO_CALLBACK,
        [](){ return std::make_unique<TOpenCurrentVideoCallback>(); }},
    {QUASAR_OPEN_CURRENT_TRAILER,
        [](){ return std::make_unique<TOpenCurrentTrailerIntent>(); }},
    {VIDEO_COMMAND_SHOW_VIDEO_SETTINGS,
        [](){ return std::make_unique<TShowVideoSettingsIntent>(); }},
    {VIDEO_COMMAND_SKIP_VIDEO_FRAGMENT,
        [](){ return std::make_unique<TSkipVideoFragmentIntent>(); }},
    {VIDEO_COMMAND_CHANGE_TRACK,
        [](){ return std::make_unique<TChangeTrackIntent>(); }},
    {VIDEO_COMMAND_CHANGE_TRACK_HARDCODED,
        [](){ return std::make_unique<TChangeTrackHardcodedIntent>(); }},
    {VIDEO_COMMAND_VIDEO_HOW_LONG,
        [](){ return std::make_unique<TVideoHowLongIntent>(); }},
    {QUASAR_VIDEO_PLAYER_FINISHED,
        [](){ return std::make_unique<TVideoFinishedTrackIntent>(); }},
};

void AddSilentSlot(NSc::TArray &slots) {
    auto slot = MakeJsonSlot(SLOT_SILENT_RESPONSE, SLOT_BOOL_TYPE, /* optional= */ true, /* value= */ true);
    slots.emplace_back(std::move(slot));
}

} // namespace

// ---------------------------------------------------------------------------
// --------------------- Allowed screens for the intents ---------------------
const TSet<EScreenId> TSearchVideoIntent::AllowedScreens = TSet<EScreenId>{};

const TSet<EScreenId> TVideoSelectionIntentBase::AllowedScreens = {
        EScreenId::WebviewVideoEntityWithCarousel,
        EScreenId::WebviewVideoEntityRelated,
        EScreenId::SeasonGallery,
        EScreenId::WebviewVideoEntitySeasons,
        EScreenId::Gallery,
        EScreenId::WebViewFilmsSearchGallery,
        EScreenId::WebViewVideoSearchGallery,
        EScreenId::TvGallery,
        EScreenId::WebViewChannels,
        EScreenId::TvExpandedCollection,
        EScreenId::SearchResults
};

const TSet<EScreenId> TOpenCurrentVideoIntentBase::AllowedScreens = {
        EScreenId::Payment,
        EScreenId::Description,
        EScreenId::ContentDetails,
        EScreenId::WebViewVideoEntity,
        EScreenId::WebviewVideoEntityWithCarousel,
        EScreenId::WebviewVideoEntityDescription,
        EScreenId::WebviewVideoEntityRelated,
        EScreenId::VideoPlayer,
        EScreenId::SeasonGallery,
        EScreenId::WebviewVideoEntitySeasons
};

const TSet<EScreenId> TGoToVideoScreenIntent::AllowedScreens = {
        EScreenId::Main,
        EScreenId::MordoviaMain,
        EScreenId::TvMain
};

const TSet<EScreenId> TPaymentConfirmedIntentBase::AllowedScreens = {
        EScreenId::Description,
        EScreenId::ContentDetails,
        EScreenId::SeasonGallery,
        EScreenId::WebViewVideoEntity,
        EScreenId::WebviewVideoEntityWithCarousel,
        EScreenId::WebviewVideoEntityDescription,
        EScreenId::WebviewVideoEntitySeasons
};

const TSet<EScreenId> TAuthorizeVideoProviderIntent::AllowedScreens = {
        EScreenId::Description,
        EScreenId::ContentDetails,
        EScreenId::WebViewVideoEntity,
        EScreenId::WebviewVideoEntityWithCarousel,
        EScreenId::WebviewVideoEntityDescription
};

const TSet<EScreenId> TPlayerOnlyVideoIntent::AllowedScreens = {
        EScreenId::VideoPlayer
};

const TSet<EScreenId> TChangeTrackHardcodedIntent::AllowedScreens = {};

const TSet<EScreenId> TOpenCurrentTrailerIntent::AllowedScreens = {
        EScreenId::WebviewVideoEntityWithCarousel,
        EScreenId::WebviewVideoEntityRelated,
        EScreenId::WebViewVideoEntity,
        EScreenId::WebviewVideoEntityDescription
};

const TSet<EScreenId> TVideoFinishedTrackIntent::AllowedScreens = {
        EScreenId::VideoPlayer
};

// --------------------------------------------------------
// --------------------- TVideoIntent ---------------------
template <class TIntent>
bool TVideoIntent<TIntent>::ShouldIntentHaveSlots() const {
    return SCENARIOS_WITH_SLOTS.contains(GetName());
}

template <class TIntent>
TResultValue TVideoIntent<TIntent>::MakeRunRequest(const NAlice::NScenarios::TScenarioRunRequest& request,
                                          NSc::TValue& resultRequest) const
{
    NSc::TValue bassRequest;

    NSc::TValue form;
    if (auto err = ConstructFormFromRequest(request, form); err.Defined()) {
        return std::move(*err);
    }
    resultRequest["form"] = std::move(form);

    if (auto dataSources = ConstructDataSourcesFromRequest(request); !dataSources.IsNull()) {
        resultRequest["data_sources"] = std::move(dataSources);
    }

    return ResultSuccess();
}

template <class TIntent>
NBASS::TResultValue TVideoIntent<TIntent>::ConstructSlotsFromSemanticFrames(const TScenarioRunRequest& request,
                                                                   TStringBuf formName,
                                                                   NSc::TArray& slots) const
{
    const auto* scenarioFrame = TryGetIntentFrame(request, formName);
    if (!scenarioFrame) {
        return TError{TError::EType::VIDEOPROTOCOLERROR, TString::Join("No frame for intent ", GetName())};
    }
    THashSet<TString> usedSlots;
    const auto* scenarioTextFrame = TryGetIntentFrame(request, TString{formName} + "_text");
    for (const auto& frameSlot : scenarioFrame->GetSlots()) {
        NSc::TValue slot;
        if (auto err = ConstructVideoSlot(frameSlot, slot); err.Defined()) {
            return std::move(*err);
        }
        if (scenarioTextFrame != nullptr) {
            if (const auto* sourceTextSlot = TryGetSlotFromFrame(*scenarioTextFrame, frameSlot.GetName())) {
                usedSlots.insert(frameSlot.GetName());
                slot["source_text"] = sourceTextSlot->GetValue();
            }
        }

        slots.emplace_back(std::move(slot));
    }

    //crunch for fixing search requests
    //if there's a slot in _text frame, but not in original frame then we skip it in building search query
    //so let put them as special text slots and use in search query
    if (scenarioTextFrame != nullptr) {
        for (const auto &frameSlot : scenarioTextFrame->GetSlots()) {
            if (usedSlots.find(frameSlot.GetName()) != usedSlots.end()) {
                continue;
            }
            NSc::TValue slot;
            if (auto err = ConstructVideoSlot(frameSlot, slot, frameSlot.GetName() + "_text"); err.Defined()) {
                return std::move(*err);
            }
            slot["source_text"] = frameSlot.GetValue();
            slots.emplace_back(std::move(slot));
        }
    }
    //finally try to use alice.quasar.video_play_text frame with raw slots
    if (formName == SEARCH_VIDEO) {
        const auto* videoRawFrame = TryGetIntentFrame(request, SEARCH_VIDEO_RAW);
        if (videoRawFrame != nullptr) {
            for (const auto& frameSlot: videoRawFrame->GetSlots()) {
                NSc::TValue slot;
                if (auto err = ConstructVideoSlot(frameSlot, slot, frameSlot.GetName() + TString{SLOT_RAW_SUFFIX}); err.Defined()) {
                    return std::move(*err);
                }
                slot["source_text"] = frameSlot.GetValue();
                slots.emplace_back(std::move(slot));
            }
        }
    }

    return ResultSuccess();
}

static const NAlice::NMegamind::TRequiredMessages requiredProtoMessages; // мегакостыль ALICE-19812

template <class TIntent>
NSc::TValue TVideoIntent<TIntent>::ConstructDataSourcesFromRequest(const TScenarioRunRequest& request) const {
    NSc::TValue result;
    for (const auto& [key, value] : request.GetDataSources()) {
        result[ToString(key)] = NSc::TValue::FromJsonValue(JsonFromProto(value));
    }
    return result;
}

template <class TIntent>
TResultValue TVideoIntent<TIntent>::ConstructFormFromRequest(const TScenarioRunRequest& request, NSc::TValue& form,
                                                    TMaybe<TStringBuf> forceFormName) const
{
    auto formName = forceFormName ? *forceFormName : GetName();
    if (!formName) {
        return TError{TError::EType::VIDEOPROTOCOLERROR, TString::Join("Cannot get form name for intent ", GetName())};
    }

    form["name"] = GetName();
    NSc::TArray& slots = form["slots"].GetArrayMutable();

    if (request.GetInput().GetEventCase() != NScenarios::TInput::kCallback) { // voice and other input types
        if (auto err = ConstructSlotsFromSemanticFrames(request, formName, slots); err.Defined()) {
            return std::move(*err);
        }
    } else { // callbacks
        if (auto err = ConstructSlotsFromCallbackPayload(request, slots); err.Defined()) {
            return std::move(*err);
        }
    }

    if (ShouldIntentHaveSlots() && slots.empty()) {
        TString errMsg = TString::Join("No slots for intent ", GetName());
        return TError{TError::EType::PROTOCOL_IRRELEVANT, errMsg};
    }

    if (IsIntentScreenDependant()) {
        const auto& deviceState = DeviceState(request);
        const auto& currentScreen = CurrentScreenId(deviceState);
        if (!IsDeviceStateWellFormed(currentScreen, deviceState) &&
            !request.GetBaseRequest().GetExperiments().fields().contains(TString{FLAG_VIDEO_DISABLE_IRREL_IF_DEVICE_STATE_MALFORMED}))
        {
            TString errMsg = TString::Join("Device state malformed for intent ", GetName(), " on ", ToString(currentScreen), " screen ");
            return TError{TError::EType::PROTOCOL_IRRELEVANT, errMsg};
        }
    }

    UpgradeForm(form);

    return ResultSuccess();
}

// --------------------------------------------------------------------
// --------------------- TVideoSelection* intents ---------------------
TResultValue TVideoSelectionIntentBase::MakeRunRequest(const NAlice::NScenarios::TScenarioRunRequest& request,
                                                       NSc::TValue& resultRequest) const {
    NSc::TValue bassRequest;

    NSc::TValue form;
    if (auto err = ConstructFormFromRequest(request, form, TryGetFormName(request)); err.Defined()) {
        return std::move(*err);
    }

    ConstructSlotsFromSelectionResult(request, form);
    resultRequest["form"] = std::move(form);

    if (auto dataSources = ConstructDataSourcesFromRequest(request); !dataSources.IsNull()) {
        resultRequest["data_sources"] = std::move(dataSources);
    }

    return ResultSuccess();
}

void TQuasarSelectVideoFromGalleryIntent::ConstructSlotsFromSelectionResult(const NAlice::NScenarios::TScenarioRunRequest& request,
                                                                            NSc::TValue& form) const
{
    auto selectFormName = TQuasarSelectVideoFromGalleryIntent::TryGetFormName(request);
    if (selectFormName == QUASAR_SELECT_VIDEO_FROM_GALLERY_BY_REMOTE_CONTROL) {
        return; // nothing to do here: all necessary slots have been copied already
    }

    TItemSelectionResult selected;
    if (IsBegemotItemSelectorEnabled(request)) {
        selected = GetBegemotItemSelectorResult(request);
    } else {
        const auto* scenarioFrame = TryGetIntentFrame(request, *selectFormName);
        const auto* slotVideoText = TryGetSlotFromFrame(*scenarioFrame, SLOT_VIDEO_TEXT);
        const TString& videoText = slotVideoText->GetValue();
        selected = SelectVideoFromGallery(request.GetBaseRequest().GetDeviceState(), videoText, TBassLogAdapter{});
    }

    NSc::TValue slot = MakeJsonSlot(SLOT_VIDEO_INDEX, NAlice::SLOT_NUM_TYPE, /* optional= */ false, /* value= */ selected.Index);
    form["slots"].Push(slot);
}

TResultValue TQuasarSelectVideoFromGalleryCallback::ConstructSlotsFromCallbackPayload(const TScenarioRunRequest& request,
                                                                                      NSc::TArray& slots) const
{
    const auto& payload = request.GetInput().GetCallback().GetPayload();
    TMordoviaJsCallbackPayload jsCallbackPayload(payload);

    /* will construct this kind of slots:
     * {
     *   "name": "video_index",
     *   "type": "num",
     *   "value": int
     *   "optional": false
     * },
     * {
     *   "name": "action"
     *   "type": "video_selection_action",
     *   "value": "play"/"description"/"list_seasons"
     *   "optional": true
     * },
     * {
     *   "name": "silent_response"
     *   "type": "bool",
     *   "value": bool
     *   "optional": true
     * }
     */
    NSc::TValue indexSlot = MakeJsonSlot(SLOT_VIDEO_INDEX, NAlice::SLOT_NUM_TYPE, /* optional= */ false,
        /* value= */ static_cast<int>(jsCallbackPayload.Payload()[TString(SLOT_VIDEO_INDEX)].GetIntNumber(-1)));
    slots.emplace_back(std::move(indexSlot));

    if (const auto& action = jsCallbackPayload.Payload().TrySelect(TString(SLOT_ACTION)); !action.IsNull()) {
        NSc::TValue actionSlot = MakeJsonSlot(SLOT_ACTION, SLOT_SELECTION_ACTION_TYPE, /* optional= */ true,
            /* value= */ action.GetString());
        slots.emplace_back(std::move(actionSlot));
    }
    AddSilentSlot(slots);

    return ResultSuccess();
}

// -----------------------------------------------------------------------
// --------------------- TOpenCurrentVideo* intents  ---------------------
TResultValue TOpenCurrentVideoCallback::ConstructSlotsFromCallbackPayload(const TScenarioRunRequest& request,
                                                                          NSc::TArray &slots) const
{
    const auto& payload = request.GetInput().GetCallback().GetPayload();
    TMordoviaJsCallbackPayload jsCallbackPayload(payload);

    /* will construct this kind of slots:
     * {
     *   "name": "action",
     *   "type": "video_selection_action",
     *   "value": "play"/"list_seasons"
     *   "optional": false
     * },
     * {
     *   "name": "silent_response"
     *   "type": "bool",
     *   "value": bool
     *   "optional": true
     * }
     */
    NSc::TValue slot = MakeJsonSlot(SLOT_ACTION, SLOT_SELECTION_ACTION_TYPE, /* optional= */ false,
                                   /* value= */ jsCallbackPayload.Payload()[TString(SLOT_ACTION)].GetString());
    slots.emplace_back(std::move(slot));

    AddSilentSlot(slots);

    return ResultSuccess();
}

// ---------------------------------------------------------
//  --------------------- CreateIntent ---------------------
std::unique_ptr<IVideoIntent> CreateIntent(TStringBuf intentType) {
    if (const auto* creator = INTENT_FACTORY.FindPtr(intentType)) {
        return (*creator)();
    }
    return {};
}

} // namespace NVideoProtocol
