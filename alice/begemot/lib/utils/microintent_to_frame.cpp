#include "microintent_to_frame.h"

#include <util/string/cast.h>

namespace NBg {

namespace {

const TString NAME_SLOT_NAME = "name";
const TString CONFIDENCE_SLOT_NAME = "confidence";
const TString MICROINTENTS_FRAME_NAME = "alice.microintents";

NAlice::TSemanticFrame::TSlot AddNameSlot(const NProto::TAliceMicrointentsResult& microintentsResult) {
    NAlice::TSemanticFrame::TSlot nameSlot;
    nameSlot.SetName(NAME_SLOT_NAME);
    nameSlot.SetType("string");
    nameSlot.SetValue(microintentsResult.GetMicrointent());
    return nameSlot;
}

NAlice::TSemanticFrame::TSlot AddConfidenceSlot(const NProto::TAliceMicrointentsResult& microintentsResult) {
    NAlice::TSemanticFrame::TSlot  confidenceSlot;
    confidenceSlot.SetName(CONFIDENCE_SLOT_NAME);
    confidenceSlot.SetType("float");
    confidenceSlot.SetValue(FloatToString(microintentsResult.GetScore()));
    return confidenceSlot;
}

} // namespace

bool BuildMicrointentsFrame(const NProto::TAliceMicrointentsResult& microintentsResult, NAlice::TSemanticFrame& resultFrame) {
    if (!microintentsResult.HasMicrointent()) {
        return false;
    }
    resultFrame.SetName(MICROINTENTS_FRAME_NAME);
    *resultFrame.AddSlots() = AddNameSlot(microintentsResult);
    *resultFrame.AddSlots() = AddConfidenceSlot(microintentsResult);
    return true;
}

} // namespace NBg
