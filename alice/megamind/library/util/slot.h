#pragma once

#include <alice/megamind/protos/common/frame.pb.h>

#include <util/generic/maybe.h>
#include <util/generic/vector.h>

namespace NAlice {

TMaybe<TSemanticFrame::TSlot> GetRequestedSlot(const TSemanticFrame& frame);

TMaybe<TSemanticFrame::TSlot> GetSlot(const TSemanticFrame& frame, const TStringBuf name);

TMaybe<TSemanticFrame::TSlot> GetFilledRequestedSlot(
    const TVector<TSemanticFrame>& requestSemanticFrames,
    const TMaybe<TSemanticFrame>& prevResponseFrame
);

} // namespace NAlice
