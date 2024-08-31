#pragma once

#include <alice/hollywood/library/framework/framework.h>
#include <alice/megamind/protos/scenarios/request.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>
#include <alice/vins/api/vins_api/speechkit/connectors/protocol/protos/state.pb.h>

namespace NAlice::NHollywoodFw {

const NScenarios::TScenarioResponseBody* GetResponseBody(const NScenarios::TScenarioRunResponse& vinsRunResponse);
NScenarios::TScenarioResponseBody* GetResponseBody(NScenarios::TScenarioRunResponse& vinsRunResponse);

const NScenarios::TScenarioResponseBody* GetResponseBody(const NScenarios::TScenarioApplyResponse& vinsApplyResponse);
NScenarios::TScenarioResponseBody* GetResponseBody(NScenarios::TScenarioApplyResponse& vinsApplyResponse);

void SaveHwfState(const NScenarios::TScenarioRunResponse& src, NScenarios::TScenarioRunResponse& dst);
void SaveHwfState(const NScenarios::TScenarioApplyResponse& src, NScenarios::TScenarioApplyResponse& dst);

template <class TScenarioStateProto, class TScenarioRequest>
void UnpackVinsState(const TStorage& hwfStorage, TScenarioRequest& requestProto) {
    TScenarioStateProto scenarioState;
    const auto getStateResult = hwfStorage.GetScenarioState(scenarioState);
    if (getStateResult == TStorage::EScenarioStateResult::Present && scenarioState.HasVinsState()) {
        requestProto.MutableBaseRequest()->MutableState()->PackFrom(scenarioState.GetVinsState());
    } else if (requestProto.GetBaseRequest().HasState()) {
        requestProto.MutableBaseRequest()->ClearState();
    }
}

template <class TScenarioStateProto>
void PackVinsState(TStorage& hwfStorage, const NScenarios::TScenarioResponseBody& vinsResponseBody) {
    TScenarioStateProto scenarioState;
    const auto getStateResult = hwfStorage.GetScenarioState(scenarioState);
    if (getStateResult != TStorage::EScenarioStateResult::Present) {
        scenarioState = TScenarioStateProto();
    }

    if (!vinsResponseBody.GetState().UnpackTo(scenarioState.MutableVinsState())) {
        scenarioState.ClearVinsState();
    }

    hwfStorage.SetScenarioState(scenarioState);
}

}
