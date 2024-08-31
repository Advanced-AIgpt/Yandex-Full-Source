#include "frame.h"

namespace NAlice::NHollywood {

TSemanticFrame TFrame::ToProto() const {
    TSemanticFrame semanticFrame;
    semanticFrame.SetName(Name_);

    for (const auto& formSlot : Slots()) {
        auto& slot = *semanticFrame.AddSlots();
        slot.SetName(formSlot.Name);
        slot.SetType(formSlot.Type);
        slot.SetIsRequested(formSlot.IsRequested);
        slot.SetIsFilled(formSlot.IsFilled);
        if (formSlot.AcceptedTypes.empty() && !formSlot.Type.empty()) {
            slot.AddAcceptedTypes(formSlot.Type);
        } else {
            for (const auto& type : formSlot.AcceptedTypes) {
                slot.AddAcceptedTypes(type);
            }
        }

        if (const auto value = formSlot.Value.AsString()) {
            slot.SetValue(value);

            auto& typedValue = *slot.MutableTypedValue();
            typedValue.SetType(formSlot.Type);
            typedValue.SetString(value);
        }
    }

    return semanticFrame;
}

TFrame TFrame::FromProto(const TSemanticFrame& semanticFrame) {
    TFrame frame{semanticFrame.GetName()};

    for (const auto& slot : semanticFrame.GetSlots()) {
        TString type;
        TString value;
        if (slot.HasTypedValue()) {
            const auto& typedValue = slot.GetTypedValue();
            type = typedValue.GetType() ? typedValue.GetType() : slot.GetType();
            value = typedValue.GetString();
        } else {
            type = slot.GetType();
            value = slot.GetValue();
        }

        TVector<TString> acceptedTypes(Reserve(slot.AcceptedTypesSize()));
        for (const auto& acceptedType : slot.GetAcceptedTypes()) {
            acceptedTypes.push_back(acceptedType);
        }

        // NOTE(a-square): eventually, other basic types may appear, consult the0@
        frame.AddSlot(TSlot{slot.GetName(),
                            std::move(type),
                            TSlot::TValue{std::move(value)},
                            std::move(acceptedTypes),
                            slot.GetIsRequested(),
                            slot.GetIsFilled()});
    }

    return frame;
}

} // namespace NAlice::NHollywood
