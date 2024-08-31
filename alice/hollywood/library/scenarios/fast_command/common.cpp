#include "common.h"

namespace NAlice::NHollywood {

namespace {

void FillDirectiveValueFromSlots(NJson::TJsonValue& directiveValue, const TStringBuf field, const TList<TSlot>* slots) {
    if (slots) {
        for (const auto& slot : *slots) {
            directiveValue[field].AppendValue(slot.Value.AsString());
        }
    }
}

}

void FillAnalyticsInfo(TResponseBodyBuilder& bodyBuilder, TStringBuf intent, const TString& productScenarioName) {
    auto& analyticsInfoBuilder = bodyBuilder.GetOrCreateAnalyticsInfoBuilder();
    analyticsInfoBuilder.SetIntentName(TString{intent});
    analyticsInfoBuilder.SetProductScenarioName(TString{productScenarioName});
}

void AddRedirectToLocation(NJson::TJsonValue& directiveValue, const TFrame& frame) {
    if (const auto locationSlot = frame.FindSlot(LOCATION_SLOT_NAME)) {
        const auto locationId = locationSlot->Value.AsString();
        directiveValue[LOCATION_ID] = (locationId == EVERYWHERE_LOCATION_SLOT) ? ALL_ROOMS_ROOM_ID : locationId;
    }

    FillDirectiveValueFromSlots(directiveValue, LOCATION_GROUPS_IDS, frame.FindSlots(LOCATION_GROUP_SLOT_NAME));
    FillDirectiveValueFromSlots(directiveValue, LOCATION_DEVICES_IDS, frame.FindSlots(LOCATION_DEVICE_SLOT_NAME));
    FillDirectiveValueFromSlots(directiveValue, LOCATION_ROOMS_IDS, frame.FindSlots(LOCATION_ROOM_SLOT_NAME));
    FillDirectiveValueFromSlots(directiveValue, LOCATION_SMART_SPEAKER_MODELS, frame.FindSlots(LOCATION_SMART_SPEAKER_MODEL_SLOT_NAME));

    if (frame.FindSlot(LOCATION_EVERYWHERE_SLOT_NAME)) {
        directiveValue[LOCATION_EVERYWHERE_SLOT_NAME] = true;
    }
}

} // namespace NAlice::NHollywood
