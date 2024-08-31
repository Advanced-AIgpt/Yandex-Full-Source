#pragma once

//
// This file required for old scenario BEFORE migration to new framework
// You have to replace all ScenarioState I/O operations in a few weeks before migration
//
// Please check https://docs.yandex-team.ru/alice-scenarios/hollywood_v2/compatibility/intro.md for more details
//

#include <alice/hollywood/library/framework/proto/framework_state.pb.h>

#include <alice/megamind/protos/scenarios/request.pb.h>
#include <google/protobuf/any.pb.h>

namespace NAlice::NHollywood {

//
// Forward declaration
//
struct TScenarioNewContext;

//
// Use this function to READ protobuf from the scenario state
//
template <class TScenarioStateProto>
bool ReadScenarioState(const NScenarios::TScenarioBaseRequest& baseRequest, TScenarioStateProto& state) {
    const auto& rawState = baseRequest.GetState();
    NHollywoodFw::TProtoHwFramework frameworkState;
    if (rawState.Is<NHollywoodFw::TProtoHwFramework>() && rawState.UnpackTo(&frameworkState)) {
        // Unpack from TProtoHwFramework::ScenarioState (support new framework)
        const auto& rawState2 = frameworkState.GetScenarioState();
        if (rawState2.Is<TScenarioStateProto>() && rawState2.UnpackTo(&state)) {
            return true;
        }
    } else if (rawState.Is<TScenarioStateProto>()) {
        // Unpack from TBaseRequestProto::State (old schema)
        return rawState.UnpackTo(&state);
    }
    // No compatible scenario state found
    return false;
}

//
// Use this function to READ protobuf from the continue/commit/apply arguments
//
inline google::protobuf::Any ReadArguments(const NScenarios::TScenarioApplyRequest& applyRequest) {
    const auto& rawState = applyRequest.GetArguments();
    NHollywoodFw::TProtoHwSceneCCAArguments frameworkArguments;
    if (rawState.Is<NHollywoodFw::TProtoHwSceneCCAArguments>() && rawState.UnpackTo(&frameworkArguments)) {
        // Unpack from TProtoHwFramework::ScenarioState (support new framework)
        return frameworkArguments.GetScenarioArgs();
    }
    return rawState;
}

template <class TScenarioArgumentsProto>
bool ReadArguments(const NScenarios::TScenarioApplyRequest& applyRequest, TScenarioArgumentsProto& state) {
    const auto& rawState = ReadArguments(applyRequest);
    if (rawState.Is<TScenarioArgumentsProto>()) {
        // Unpack from TBaseRequestProto::State (old schema)
        return rawState.UnpackTo(&state);
    }
    // No compatible scenario state found
    return false;
}

//
// Use this function to wrap scenario answers when old flow called with TReturnValueDo()
// but new continue/apply handler must forward calls into old flow too
//
// `TScenarioNewContext` is a member (`NewContext`) of struct `TScenarioHandleContext`
//
google::protobuf::Any PrepareArguments(const google::protobuf::Any& scenarioArgs, const TScenarioNewContext* newContext);
template <class TScenarioArgumentsProto>
google::protobuf::Any PrepareArguments(const TScenarioArgumentsProto& scenarioArgs, const TScenarioNewContext* newContext) {
    google::protobuf::Any scenarioArgsPacked;
    scenarioArgsPacked.PackFrom(scenarioArgs);
    return PrepareArguments(scenarioArgsPacked, newContext);
}

google::protobuf::Any PrepareOldFlowArguments(const google::protobuf::Any& scenarioArgs);
template <class TScenarioArgumentsProto>
google::protobuf::Any PrepareOldFlowArguments(const TScenarioArgumentsProto& scenarioArgs) {
    google::protobuf::Any scenarioArgsPacked;
    scenarioArgsPacked.PackFrom(scenarioArgs);
    return PrepareOldFlowArguments(scenarioArgsPacked);
}

} // namespace NAlice::NHollywood
