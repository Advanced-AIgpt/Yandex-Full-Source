#include "builder.h"

#include <alice/protos/data/language/language.pb.h>

namespace NAlice {

TSemanticFrameBuilder& TSemanticFrameBuilder::SetName(const TString& name) {
    Frame.SetName(name);
    return *this;
}

TSemanticFrameBuilder& TSemanticFrameBuilder::AddSlot(
    const TString& name,
    TArrayRef<const TString> acceptedTypes,
    const TMaybe<TString>& type,
    const TMaybe<TString>& value,
    bool isRequested,
    bool isFilled
) {
    *Frame.AddSlots() = MakeSlot(name, acceptedTypes, type, value, isRequested, isFilled);
    return *this;
}

TSemanticFrameBuilder& TSemanticFrameBuilder::SetSlotValue(const TString& name, const TString& type, const TString& value) {
    for (auto& slot : *Frame.MutableSlots()) {
        if (slot.GetName() == name) {
            // TODO(the0): check accepted types
            slot.SetType(type);
            slot.SetValue(value);
            return *this;
        }
    }

    ythrow yexception() << "Trying to set value for unknown slot.";
}

TSemanticFrame TSemanticFrameBuilder::Build() const {
    return Frame;
}

TSemanticFrame::TSlot MakeSlot(const TString& name, TArrayRef<const TString> acceptedTypes,
                const TMaybe<TString>& type, const TMaybe<TString>& value, const bool isRequested, const bool isFilled) {
    TSemanticFrame::TSlot slot;
    slot.SetName(name);
    for (const auto& acceptedType : acceptedTypes) {
        slot.AddAcceptedTypes(acceptedType);
    }
    if (type.Defined()) {
        slot.SetType(*type);
    }
    if (value.Defined()) {
        slot.SetValue(*value);
    }
    slot.SetIsRequested(isRequested);
    slot.SetIsFilled(isFilled);
    return slot;
}

TClientEntity MakeEntity(const TString& name, const THashMap<TString, TArrayRef<const TString>>& items) {
    TClientEntity entity;
    entity.SetName(name);
    for (const auto& [value, phrases] : items) {
        NAlice::TNluHint nluHint;
        for (const auto& phrase : phrases) {
            NAlice::TNluPhrase nluPhrase;
            nluPhrase.SetLanguage(NAlice::ELang::L_RUS);
            nluPhrase.SetPhrase(phrase);
            *nluHint.AddInstances() = nluPhrase;
        }
        (*entity.MutableItems())[value] = nluHint;
    }
    return entity;
}

TSemanticFrame MakeFrame(
    const TString& name,
    TArrayRef<const TSemanticFrame::TSlot> slots
) {
    TSemanticFrame frame;
    frame.SetName(name);
    for (const auto& slot : slots) {
        *frame.AddSlots() = slot;
    }
    return frame;
}

} // namespace NAlice
