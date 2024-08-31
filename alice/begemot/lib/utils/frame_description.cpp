#include "frame_description.h"

namespace NBg {

NAlice::TFrameDescriptionMap ReadFrameDescriptionMap(const NProto::TGranetConfig& config, bool taggerOnly) {
    NAlice::TFrameDescriptionMap result;
    for (const NProto::TGranetConfig::TForm& form : config.GetForms()) {
        Y_ASSERT(!result.contains(form.GetName()));
        if (taggerOnly && !form.GetEnableAliceTagger()) {
            continue;
        }
        for (const NProto::TGranetConfig::TSlot& slot : form.GetSlots()) {
            Y_ASSERT(!result[form.GetName()].Slots.contains(slot.GetName()));
            TVector<TString> types;
            for (const TString& type : slot.GetAcceptedTypes()) {
                types.push_back(type);
            }
            result[form.GetName()].Slots.insert({
                slot.GetName(),
                NAlice::TFrameDescription::TSlot{slot.GetName(), types, slot.GetConcatenateStrings(), slot.GetKeepVariants()}
            });
        }
    }
    return result;
}

} // namespace NBg
