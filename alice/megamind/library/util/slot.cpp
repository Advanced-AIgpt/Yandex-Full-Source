#include "slot.h"

namespace NAlice {

TMaybe<TSemanticFrame::TSlot> GetRequestedSlot(const TSemanticFrame& frame) {
    for (const auto& slot : frame.GetSlots()) {
        if (slot.GetIsRequested()) {
            return slot;
        }
    }
    return Nothing();
}

TMaybe<TSemanticFrame::TSlot> GetSlot(const TSemanticFrame& frame, const TStringBuf name) {
    for (const auto& slot : frame.GetSlots()) {
        if (slot.GetName() == name) {
            return slot;
        }
    }
    return Nothing();
}

TMaybe<TSemanticFrame::TSlot> GetFilledRequestedSlot(
    const TVector<TSemanticFrame>& requestSemanticFrames,
    const TMaybe<TSemanticFrame>& prevResponseFrame
) {
    if (!prevResponseFrame.Defined()) {
        return Nothing();
    }

    const auto requestedSlot = GetRequestedSlot(*prevResponseFrame);
    if (!requestedSlot.Defined()) {
        return Nothing();
    }
    for (const auto& requestFrame : requestSemanticFrames) {
        if (requestFrame.GetName() == prevResponseFrame->GetName()) {
            const TMaybe<TSemanticFrame::TSlot> slot = GetSlot(requestFrame, requestedSlot->GetName());
            if (slot.Defined() && (!slot->GetType().empty() || slot->HasTypedValue())) {
                return *slot;
            }
        }
    }
    return Nothing();
}

} // namespace NAlice
