#pragma once

#include <dj/services/alisa_skills/server/proto/data/data_types.pb.h>

namespace NAlice {

bool SlotValueSatisfiesCondition(const TString& actualValueString, const TString& expectedValueString);

bool SlotSatisfiesCondition(const TSemanticFrame::TSlot& actual, const TSemanticFrame::TSlot& expected);

bool FrameSatisfiesCondition(const TSemanticFrame& actual, const TSemanticFrame& expected);

bool AnyFrameSatisfiesCondition(const TVector<TSemanticFrame>& actualFrames, const TSemanticFrame& expected);

bool DeviceStateSatisfiesCondition(const TDeviceState& actualState, const TDeviceState& expectedState);

bool IsValidSuccessCondition(const NDJ::NAS::TSuccessCondition& successCondition);

bool HasValidSuccessCondition(const NDJ::NAS::TItemAnalytics& analytics);

} // NAlice
