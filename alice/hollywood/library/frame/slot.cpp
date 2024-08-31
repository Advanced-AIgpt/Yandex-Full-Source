#include "slot.h"

namespace NAlice::NHollywood {

TSemanticFrame::TSlot CreateProtoSlot(const TString& name, const TString& type, const TString& value) {
    TSemanticFrame::TSlot slot;
    slot.SetName(name);
    slot.SetValue(value);
    slot.SetType(type);
    slot.AddAcceptedTypes(type);
    return slot;
}

} // NAlice::NHollywood
