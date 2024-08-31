#pragma once

#include <alice/megamind/protos/common/frame.pb.h>
#include <alice/megamind/protos/scenarios/request.pb.h>
#include <alice/library/video_common/defs.h>

#include <alice/bass/libs/video_common/item_selector.h>
#include <alice/bass/util/error.h>

#include <library/cpp/scheme/scheme.h>

namespace NVideoProtocol {

NSc::TValue MakeJsonSlot(TStringBuf name, TStringBuf type, bool optional, NSc::TValue value);

NBASS::TResultValue ConstructVideoSlot(const NAlice::TSemanticFrame::TSlot& slot, NSc::TValue& resultSlot, const TStringBuf explicitSlotName = "");

const NAlice::TDeviceState& DeviceState(const NAlice::NScenarios::TScenarioRunRequest& request);

const NAlice::TSemanticFrame* TryGetIntentFrame(const NAlice::NScenarios::TScenarioRunRequest& request,
                                                TStringBuf intentName);
bool HasIntentFrame(const NAlice::NScenarios::TScenarioRunRequest& request, TStringBuf intentName);

const NAlice::TSemanticFrame::TSlot* TryGetSlotFromFrame(const NAlice::TSemanticFrame& frame,
                                                         TStringBuf slotName);

bool IsBegemotItemSelectorEnabled(const NAlice::NScenarios::TScenarioRunRequest& request);

NBASS::NVideoCommon::TItemSelectionResult SelectByName(const NAlice::NScenarios::TScenarioRunRequest& request);
NBASS::NVideoCommon::TItemSelectionResult SelectByNumber(const NAlice::NScenarios::TScenarioRunRequest& request);
NBASS::NVideoCommon::TItemSelectionResult GetBegemotItemSelectorResult(const NAlice::NScenarios::TScenarioRunRequest& request);

} // namespace NAlice::NVideo
