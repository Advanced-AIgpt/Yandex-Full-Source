#include "slot.h"

#include <util/stream/output.h>

template <>
void Out<NAlice::TSlot>(IOutputStream& out, const NAlice::TSlot& slot) {
    out << '{' << slot.Name << ", " << slot.Type << ", " << slot.Value.AsString() << ", " << slot.SourceText << '}';
}

template <>
void Out<NAlice::TSlotMap>(IOutputStream& out, const NAlice::TSlotMap& slots) {
    bool first = true;
    out << '{';
    for (const auto& [slotName, slot] : slots) {
        if (!first)
            out << ", ";
        out << '{' << slotName << ": " << slot << '}';
        first = false;
    }
    out << '}';
}

namespace NAlice {

TSemanticFrame ToSemanticFrame(const TString& name, const NAlice::TSlotMap& slotMap) {
    NAlice::TSemanticFrame frame;
    frame.SetName(name);
    for (const auto& [slotName, formSlot] : slotMap) {
        auto& slot = *frame.AddSlots();
        slot.SetName(formSlot.Name);
        slot.SetType(formSlot.Type);
        slot.AddAcceptedTypes(formSlot.Type);

        if (const auto value = formSlot.Value.AsString()) {
            slot.SetValue(value);
        }
    }
    return frame;
}

TSlot::TSlot(const TString& name, const TString& type, const TString& value, const TMaybe<TString>& sourceText,
             const TTokenRange& range)
    : Name(name)
    , Type(type)
    , Value(value)
    , SourceText(sourceText)
    , Range(range)
{
}

} // namespace NAlice
