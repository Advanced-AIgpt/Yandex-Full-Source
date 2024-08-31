#include "utils.h"

#include <alice/library/frame/description.h>
#include <alice/library/json/json.h>
#include <alice/library/video_common/defs.h>
#include <alice/library/video_common/device_helpers.h>
#include <alice/library/video_common/mordovia_webview_helpers.h>

#include <alice/megamind/library/experiments/flags.h>
#include <alice/megamind/protos/common/data_source_type.pb.h>
#include <alice/megamind/protos/scenarios/begemot.pb.h>
#include <alice/protos/data/search_result/search_result.pb.h>

#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/util/error.h>

using namespace NAlice::NVideoCommon;
using namespace NBASS::NVideoCommon;

namespace {

const THashMap<TStringBuf, TStringBuf> BEGEMOT_TAGS_TO_BASS_SLOTS{{SLOT_SCREEN_NAME, SLOT_SCREEN}};

const THashMap<TStringBuf, TStringBuf> GRANET_SLOT_TYPES_TO_BASS_SLOT_TYPES{
    {SLOT_CUSTOM_SCREEN_TYPE, SLOT_SCREEN_TYPE},
    {SLOT_CUSTOM_SELECTION_ACTION_TYPE, SLOT_SELECTION_ACTION_TYPE},
    {SLOT_CUSTOM_SEASON_TYPE, SLOT_SEASON_TYPE},
    {SLOT_CUSTOM_EPISODE_TYPE, SLOT_EPISODE_TYPE},
    {NAlice::SLOT_FST_NUM_TYPE, NAlice::SLOT_NUM_TYPE}};

const THashMap<TStringBuf, TStringBuf> FORM_TO_WAZARD_FORM_NAME {
    {QUASAR_VIDEO_PLAY_NEXT_VIDEO, WIZARD_QUASAR_VIDEO_PLAY_NEXT_VIDEO},
    {QUASAR_VIDEO_PLAY_PREV_VIDEO, WIZARD_QUASAR_VIDEO_PLAY_PREV_VIDEO}};

constexpr float ABSOLUTELY_SURE = 1;

} // namespace

namespace NVideoProtocol {

NSc::TValue MakeJsonSlot(TStringBuf name, TStringBuf type, bool optional, NSc::TValue value) {
    NSc::TValue slot;
    slot["name"] = name;
    slot["type"] = type;
    slot["optional"] = optional;
    slot["value"].SetNull();
    if (!value.IsNull()) {
        slot["value"].Swap(value);
    }
    return slot;
}

NBASS::TResultValue ConstructVideoSlot(const NAlice::TSemanticFrame::TSlot& slot,
                                       NSc::TValue& resultSlot, const TStringBuf explicitSlotName) {
    const TStringBuf slotName = explicitSlotName.empty() ? slot.GetName() : explicitSlotName;
    const TStringBuf slotType = slot.GetType();
    const TStringBuf slotValue = slot.GetValue();

    const auto* mappedSlot = BEGEMOT_TAGS_TO_BASS_SLOTS.FindPtr(slotName);
    const TStringBuf name = mappedSlot ? *mappedSlot : slotName;
    const auto* mappedSlotType = GRANET_SLOT_TYPES_TO_BASS_SLOT_TYPES.FindPtr(slotType);
    const TStringBuf type = mappedSlotType ? *mappedSlotType : slotType;

    NSc::TValue value;
    if (type == NAlice::SLOT_NUM_TYPE) {
        i64 ivalue = 0;
        if (TryFromString(slotValue, ivalue)) {
            value = ivalue;
        } else {
            return NBASS::TError{NBASS::TError::EType::VIDEOPROTOCOLERROR,
                                 TStringBuilder() << "Cannot convert num slot value (" << slotValue << ") to i64"};
        }
    } else if (type == NAlice::NVideoCommon::SLOT_BOOL_TYPE) {
        bool bvalue;
        if (TryFromString(slotValue, bvalue)) {
            value = bvalue;
        } else {
            return NBASS::TError{NBASS::TError::EType::VIDEOPROTOCOLERROR,
                                 TStringBuilder() << "Cannot convert bool slot value (" << slotValue << ") to bool"};
        }
    } else {
        value = slotValue;
    }

    if (type == NAlice::SLOT_DATE_TYPE && name == SLOT_RELEASE_DATE) {
        const NJson::TJsonValue& date = NJson::ReadJsonFastTree(slotValue);

        if (!date.Has("years")) {
            return NBASS::TError{NBASS::TError::EType::PROTOCOL_IRRELEVANT,
                "Release_date slot should contain years for video request."};
        }
        value = ToString(date["years"].GetUInteger());
    }

    resultSlot = MakeJsonSlot(name, type, /* optional= */ true, value);

    return NBASS::ResultSuccess();
}

const NAlice::TDeviceState& DeviceState(const NAlice::NScenarios::TScenarioRunRequest& request) {
    return request.GetBaseRequest().GetDeviceState();
}

const NAlice::TSemanticFrame* TryGetIntentFrame(const NAlice::NScenarios::TScenarioRunRequest& request,
                                                TStringBuf intentName) {
    if (const auto* toWizard = FORM_TO_WAZARD_FORM_NAME.FindPtr(intentName)) {
        intentName = *toWizard;
    }
    const auto& frames = request.GetInput().GetSemanticFrames();
    const auto* intentFrame = FindIfPtr(frames, [&](const NAlice::TSemanticFrame& frame) {
        return frame.GetName() == intentName;
    });
    return intentFrame;
}

bool HasIntentFrame(const NAlice::NScenarios::TScenarioRunRequest& request, TStringBuf intentName) {
    return TryGetIntentFrame(request, intentName) != nullptr;
}

const NAlice::TSemanticFrame::TSlot* TryGetSlotFromFrame(const NAlice::TSemanticFrame& frame,
                                                         TStringBuf slotName) {
    const auto* slot = FindIfPtr(frame.GetSlots(), [&](const NAlice::TSemanticFrame::TSlot& slot) {
        return slot.GetName() == slotName;
    });
    return slot;
}

bool IsBegemotItemSelectorEnabled(const NAlice::NScenarios::TScenarioRunRequest& request) {
    EScreenId currentScreen = CurrentScreenId(DeviceState(request));
    return request.GetBaseRequest().GetExperiments().fields().count(TString{NAlice::EXP_DISABLE_BEGEMOT_ITEM_SELECTOR}) == 0 &&
            (IsGalleryScreen(currentScreen) || currentScreen == EScreenId::SearchResults || currentScreen == EScreenId::TvExpandedCollection);
}

TItemSelectionResult SelectByName(const NAlice::NScenarios::TScenarioRunRequest& request) {
    if (request.GetDataSources().count(NAlice::EDataSourceType::BEGEMOT_ITEM_SELECTOR_RESULT) == 0) {
        return {};
    }
    const NAlice::NScenarios::TBegemotItemSelectorResult result = request.GetDataSources()
        .at(NAlice::EDataSourceType::BEGEMOT_ITEM_SELECTOR_RESULT).GetBegemotItemSelectorResult();
    LOG(INFO) << "Item Selector has returned: " << result << Endl;
    for (const auto& gallery : result.GetGalleries()) {
        if (gallery.GetGalleryName() == "video_gallery") {
            for (const auto& item : gallery.GetItems()) {
                if (item.GetIsSelected()) {
                    return {FromString<int>(item.GetAlias()), static_cast<float>(item.GetScore())};
                }
            }
        }
    }
    return {};
}

TItemSelectionResult SelectByNumber(const NAlice::NScenarios::TScenarioRunRequest& request) {
    for (const auto& frame : request.GetInput().GetSemanticFrames()) {
        if (frame.GetName() != "personal_assistant.scenarios.select_video_by_number") {
            continue;
        }
        for (const auto& slot : frame.GetSlots()) {
            if (slot.GetName() != "video_number") {
                continue;
            }
            return {FromString<int>(slot.GetValue()), ABSOLUTELY_SURE};
        }
    }
    return {};
}

TItemSelectionResult GetBegemotItemSelectorResult(const NAlice::NScenarios::TScenarioRunRequest& request) {
    size_t itemsCount = 0;

    const auto currentScreen = CurrentScreenId(DeviceState(request));
    if (currentScreen == EScreenId::Gallery) {
        itemsCount = request.GetBaseRequest().GetDeviceState().GetVideo().GetScreenState().RawItemsSize();
    } else if (IsWebViewGalleryScreen(currentScreen)) {
        const auto& viewStateProto = request.GetBaseRequest().GetDeviceState().GetVideo().GetViewState();
        NSc::TValue viewState = NSc::TValue::FromJson(NAlice::JsonStringFromProto(viewStateProto));
        itemsCount = GetWebViewGalleryItems(viewStateProto).values_size();
    } else if (currentScreen == EScreenId::SearchResults) {
        const NAlice::TDeviceState::TVideo::TTvInterfaceState& tvInterfaceState = request.GetBaseRequest().GetDeviceState().GetVideo().GetTvInterfaceState();
        if (tvInterfaceState.HasSearchResultsScreen()) {
            const auto searchResultsScreen = tvInterfaceState.GetSearchResultsScreen();
            for (size_t i = 0; i < searchResultsScreen.GalleriesSize(); ++i) {
                const auto& gallery = searchResultsScreen.GetGalleries(i);
                itemsCount += gallery.GetVisible() ? gallery.ItemsSize() : 0;
            }
        }
    } else if (currentScreen == EScreenId::TvExpandedCollection) {
        const NAlice::TDeviceState::TVideo::TTvInterfaceState& tvInterfaceState = request.GetBaseRequest().GetDeviceState().GetVideo().GetTvInterfaceState();
        if (tvInterfaceState.HasExpandedCollectionScreen()) {
            const auto expandedCollections = tvInterfaceState.GetExpandedCollectionScreen();
            for (size_t i = 0; i < expandedCollections.GalleriesSize(); ++i) {
                const auto& gallery = expandedCollections.GetGalleries(i);
                itemsCount += gallery.GetVisible() ? gallery.ItemsSize() : 0;
            }
        }
    }

    if (itemsCount == 0) {
        LOG(WARNING) << "There are no items in the gallery!" << Endl;
        return {};
    }

    const TItemSelectionResult selectionByName = SelectByName(request);
    const TItemSelectionResult selectionByNumber = SelectByNumber(request);
    if (1 <= selectionByName.Index && selectionByName.Index <= static_cast<int>(itemsCount)) {
        LOG(INFO) << "select_video_from_gallery triggered with name, index is: " << selectionByName.Index << Endl;
        return selectionByName;
    } else if (1 <= selectionByNumber.Index && selectionByNumber.Index <= static_cast<int>(itemsCount)) {
        LOG(INFO) << "select_video_from_gallery triggered with number, index is: " << selectionByNumber.Index << Endl;
        return selectionByNumber;
    }
    return {};
}

} // namespace NVideoProtocol
