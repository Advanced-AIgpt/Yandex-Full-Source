#include "form_to_frame.h"

#include <util/generic/is_in.h>
#include <util/string/join.h>

namespace NBg {
    NAlice::TRecognizedSlot GetRecognizedSlot(const NBg::NProto::TGranetTag& granetTag) {
        NAlice::TRecognizedSlot recognizedSlot;
        recognizedSlot.Name = granetTag.GetName();
        recognizedSlot.Begin = granetTag.GetBegin();
        recognizedSlot.End = granetTag.GetEnd();
        if (!granetTag.GetData().empty()) {
            recognizedSlot.Variants.reserve(granetTag.GetData().size());
            for (const auto& data : granetTag.GetData()) {
                auto& variant = recognizedSlot.Variants.emplace_back();
                variant.Type = data.GetType();
                if (!variant.Type.empty()) {
                    variant.Value = data.GetValue();
                }
            }
        }

        return recognizedSlot;
    }

    TVector<TString> GetGranetTokenTexts(const TVector<NProto::TGranetToken>& granetTokens) {
        TVector<TString> tokens;
        for (const auto& granetToken : granetTokens) {
            tokens.push_back(granetToken.GetText());
        }
        return tokens;
    }

    NAlice::TSemanticFrame ConvertFormToSemanticFrame(
        const NBg::NProto::TGranetForm& granetForm,
        const NAlice::TFrameDescription* description
    ) {
        NAlice::TSemanticFrame frame;
        frame.SetName(granetForm.GetName());

        for (const auto& tag : granetForm.GetTags()) {
            const NAlice::TFrameDescription::TSlot* slotDescription = description ? description->Slots.FindPtr(tag.GetName()) : nullptr;
            const NAlice::TRecognizedSlot recognizedSlot = GetRecognizedSlot(tag);
            const bool shouldKeepVariants = slotDescription ? slotDescription->KeepVariants : false;

            NAlice::TSemanticFrame::TSlot slot;
            if (shouldKeepVariants) {
                slot.SetType(ToString(NAlice::SLOT_VARIANTS_TYPE));
                slot.SetValue(NAlice::PackVariantsValue(recognizedSlot.Variants));
            } else if (!recognizedSlot.Variants.empty()) {
                slot.SetType(recognizedSlot.Variants[0].Type);
                slot.SetValue(recognizedSlot.Variants[0].Value);
            }

            if (slotDescription) {
                if (!shouldKeepVariants && !slot.GetType().empty() && !IsIn(slotDescription->Types, slot.GetType())) {
                    continue;
                }
                for (const TString& type : slotDescription->Types) {
                    slot.AddAcceptedTypes(type);
                }
            } else {
                slot.AddAcceptedTypes(slot.GetType());
            }

            slot.SetName(tag.GetName());
            *frame.AddSlots() = slot;
        }

        return frame;
    }
} // namespace NBg
