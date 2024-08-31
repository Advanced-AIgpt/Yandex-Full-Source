#pragma once

#include <alice/hollywood/library/base_hw_service/base_hw_service_handle.h>
#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/framework/framework.h>

#include <library/cpp/monlib/metrics/labels.h>

namespace NAlice::NHollywood {

enum class EMiscHandle {
    UNPACK /* "unpack" */,
    PACK /* "pack" */,
    UNPACK_MODIFIERS /* "unpack_modifiers" */,
    PACK_MODIFIERS /* "pack_modifiers" */,

    FAST_DATA_RELOAD /* fast_data_reload */,
    FAST_DATA_VERSION /* fast_data_version */,
    PING /* ping */,
    SOLOMON /* "solomon" */,
    VERSION /* version */,
    UTILITY /* utility */,
    REOPEN_LOGS /* reopen_logs */,

    SPLIT_WEB_SEARCH /* split_web_search */,
};

enum class ERequestResult {
    SUCCESS /* "success" */,
    ERROR /* "error" */,
    TOTAL /* "total" */,

    INCOMING /* incoming */,
};

enum class EScenarioStage {
    BassRender /* "bass_render" */,
    FillAnalyticsInfo /* "fill_analytics_info" */,
    FillFeatures /* "fill_features" */,
    PackApplyArgs /* "pack_apply_args" */,
    PackProto /* "pack_proto" */,
    UnpackJson /* "unpack_json" */
};

NMonitoring::TLabels ScenarioLabels(const TScenario::THandleBase& handle);
NMonitoring::TLabels ScenarioLabels(const NHollywoodFw::TScenario& scenario, const TString& handleName);
NMonitoring::TLabels SceneLabels(const NHollywoodFw::TScenario& scenario, const TString& sceneName);
NMonitoring::TLabels ScenarioResponse(const TScenario::THandleBase& handle, ERequestResult requestResult);
NMonitoring::TLabels ScenarioResponse(const NHollywoodFw::TScenario& scenario, const TString& handleName, ERequestResult requestResult);
NMonitoring::TLabels ScenarioResponseTime(const TScenario::THandleBase& handle, ERequestResult requestResult);
NMonitoring::TLabels ScenarioResponseTime(const NHollywoodFw::TScenario& scenario, const TString& handleName, ERequestResult requestResult);
NMonitoring::TLabels ScenarioStageTime(const TScenario::THandleBase& handle, EScenarioStage stage);

NMonitoring::TLabels MiscLabels(EMiscHandle handle);
NMonitoring::TLabels MiscResponse(EMiscHandle handle, ERequestResult requestResult);
NMonitoring::TLabels MiscResponseTime(EMiscHandle handle, ERequestResult requestResult);

NMonitoring::TLabels HwServiceLabels(const IHwServiceHandle& handle);
NMonitoring::TLabels HwServiceResponse(const IHwServiceHandle& handle, ERequestResult requestResult);
NMonitoring::TLabels HwServiceResponseTime(const IHwServiceHandle& handle, ERequestResult requestResult);

} // namespace NAlice::NHollywood
