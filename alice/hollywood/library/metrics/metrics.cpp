#include "metrics.h"

namespace NAlice::NHollywood {

namespace {

const TString REQUEST_RESULT{TStringBuf("request_result")};
const TString REQUEST_STAGE{TStringBuf("request_stage")};
const TString REQUEST_TYPE{TStringBuf("request_type")};
const TString SCENE_NAME{TStringBuf("scene_name")};

void AddResponseTimeLabel(NMonitoring::TLabels& labels) {
    labels.Add("name", "response_time_milliseconds");
}

} // namespace

NMonitoring::TLabels ScenarioLabels(const TScenario::THandleBase& handle) {
    return {
        {"scenario_name", handle.ScenarioName()},
        {REQUEST_TYPE, handle.Name()}
    };
}

NMonitoring::TLabels ScenarioLabels(const NHollywoodFw::TScenario& scenario, const TString& handleName) {
    return {
        {"scenario_name", scenario.GetName()},
        {REQUEST_TYPE, handleName}
    };
}

NMonitoring::TLabels SceneLabels(const NHollywoodFw::TScenario& scenario, const TString& sceneName) {
    return {
        {"scenario_name", scenario.GetName()},
        {SCENE_NAME, sceneName}
    };
}

NMonitoring::TLabels ScenarioResponse(const TScenario::THandleBase& handle, const ERequestResult requestResult) {
    auto labels = ScenarioLabels(handle);
    labels.Add(REQUEST_RESULT, ToString(requestResult));
    return labels;
}

NMonitoring::TLabels ScenarioResponse(const NHollywoodFw::TScenario& scenario, const TString& handleName, ERequestResult requestResult) {
    auto labels = ScenarioLabels(scenario, handleName);
    labels.Add(REQUEST_RESULT, ToString(requestResult));
    return labels;
}

NMonitoring::TLabels ScenarioResponseTime(const TScenario::THandleBase& handle, const ERequestResult requestResult) {
    auto labels = ScenarioResponse(handle, requestResult);
    AddResponseTimeLabel(labels);
    return labels;
}

NMonitoring::TLabels ScenarioResponseTime(const NHollywoodFw::TScenario& scenario, const TString& handleName, ERequestResult requestResult) {
    auto labels = ScenarioResponse(scenario, handleName, requestResult);
    AddResponseTimeLabel(labels);
    return labels;
}

NMonitoring::TLabels ScenarioStageTime(const TScenario::THandleBase& handle, EScenarioStage stage) {
    auto labels = ScenarioLabels(handle);
    labels.Add(REQUEST_STAGE, ToString(stage));
    return labels;
}

NMonitoring::TLabels MiscLabels(EMiscHandle handle) {
    return {
        {REQUEST_TYPE, ToString(handle)}
    };
}

NMonitoring::TLabels MiscResponse(const EMiscHandle handle, const ERequestResult requestResult) {
    auto labels = MiscLabels(handle);
    labels.Add(REQUEST_RESULT, ToString(requestResult));
    return labels;
}

NMonitoring::TLabels MiscResponseTime(const EMiscHandle handle, const ERequestResult requestResult) {
    auto labels = MiscResponse(handle, requestResult);
    AddResponseTimeLabel(labels);
    return labels;
}

NMonitoring::TLabels HwServiceLabels(const IHwServiceHandle& handle) {
    return {
        {"service_name", handle.Name()}
    };
}

NMonitoring::TLabels HwServiceResponse(const IHwServiceHandle& handle, const ERequestResult requestResult) {
    auto labels = HwServiceLabels(handle);
    labels.Add(REQUEST_RESULT, ToString(requestResult));
    return labels;
}

NMonitoring::TLabels HwServiceResponseTime(const IHwServiceHandle& handle, const ERequestResult requestResult) {
    auto labels = HwServiceResponse(handle, requestResult);
    AddResponseTimeLabel(labels);
    return labels;
}

} // namespace NAlice::NHollywood
