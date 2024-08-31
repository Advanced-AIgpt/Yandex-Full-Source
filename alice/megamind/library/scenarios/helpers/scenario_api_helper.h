#pragma once

#include "session_view.h"

#include <alice/megamind/library/context/context.h>
#include <alice/megamind/library/request/event/event.h>
#include <alice/megamind/library/scenarios/interface/data_sources.h>
#include <alice/megamind/library/scenarios/interface/scenario_env.h>
#include <alice/megamind/library/session/dialog_history.h>
#include <alice/megamind/protos/common/data_source_type.pb.h>
#include <alice/megamind/protos/common/location.pb.h>
#include <alice/megamind/protos/scenarios/request.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <alice/library/geo/user_location.h>

#include <functional>

namespace NAlice::NMegamind {

namespace NImpl {

using TUserConfigs = ::ru::yandex::alice::memento::proto::TUserConfigs;

THashMap<::ru::yandex::alice::memento::proto::EConfigKey, const google::protobuf::FieldDescriptor*>
LoadUserConfigsMapping();

void ConstructMementoUserConfigs(TUserConfigs& userConfigs, const TScenarioConfig& config,
                                 const TUserConfigs& sourceUserConfigs);

void ConstructDeviceState(TDeviceState& deviceState, const TScenarioConfig& scenarioConfig,
                          const TDeviceState& sourceDeviceState);

void AddScenarioDataSourcesByExp(const TString& scenarioName, const IContext::TExpFlags& expFlags, TVector<EDataSourceType>& scenarioDataSources);

bool DependsOnWebSearchResultByExp(const TString& scenarioName, const IContext::TExpFlags& expFlags);

void ConstructDataSources(
    ::google::protobuf::Map<::google::protobuf::int32, NScenarios::TDataSource>* dataSourcesProto,
    const TScenarioConfig& config,
    NMegamind::IDataSources& dataSources,
    IContext::TExpFlags expFlags,
    const bool dialogHistoryAllowed,
    const bool passDataSourcesInRequest);

void ConstructNluFeatures(
    ::google::protobuf::RepeatedPtrField<NScenarios::TNluFeature>& featuresProto,
    const TScenarioConfig& config,
    const TWizardResponse& wizardResponse);

void ConstructBaseRequest(
    NScenarios::TScenarioBaseRequest* request,
    const TScenarioConfig& config,
    const TSpeechKitRequest& skRequest,
    ui64 seed,
    ELanguage userLanguage,
    TScenarioSessionView session,
    TMementoDataView mementoData,
    const TRequest& requestModel,
    const TWizardResponse& wizardResponse);

void ConstructInput(
    NScenarios::TInput* input,
    const TSpeechKitRequest& skRequest,
    TArrayRef<const TSemanticFrame> semanticFrames,
    const IEvent& event,
    const TMaybe<TString>& normalizedUtterance);

NScenarios::TScenarioRunRequest ConstructRunRequest(
    const TScenarioConfig& config,
    const TSpeechKitRequest& skRequest,
    const IContext& ctx,
    NMegamind::IDataSources& dataSources,
    ui64 seed,
    ELanguage userLanguage,
    TScenarioSessionView session,
    TMementoDataView mementoData,
    TArrayRef<const TSemanticFrame> semanticFrames,
    const TRequest& requestModel,
    const TMaybe<TString>& utterance,
    const TMaybe<TString>& normalizedUtterance,
    TRTLogger& logger,
    const bool passDataSourcesInRequest);

NScenarios::TScenarioApplyRequest ConstructApplyRequest(
    const TScenarioConfig& config,
    const TSpeechKitRequest& skRequest,
    ui64 seed,
    ELanguage userLanguage,
    TScenarioSessionView sessionView,
    TMementoDataView mementoData,
    TArrayRef<const TSemanticFrame> semanticFrames,
    const google::protobuf::Any& arguments,
    const TRequest& requestModel,
    const TMaybe<TString>& utterance,
    const TMaybe<TString>& normalizedUtterance,
    const ISession* session,
    const TWizardResponse& wizardResponse);

} // namespace NAlice::NMegamind::NImpl

NScenarios::TScenarioRunRequest ConstructRunRequest(
    const TScenarioConfig& config,
    const TRequest& requestModel,
    const IContext& ctx,
    NMegamind::IDataSources& dataSources,
    TScenarioSessionView session,
    TMementoDataView mementoData,
    TArrayRef<const TSemanticFrame> frames,
    const bool passDataSourcesInRequest);

NScenarios::TScenarioApplyRequest ConstructApplyRequest(
    const TScenarioConfig& config,
    const TRequest& requestModel,
    const IContext& ctx,
    TScenarioSessionView session,
    TMementoDataView mementoData,
    TArrayRef<const TSemanticFrame> frames,
    const google::protobuf::Any& arguments);

} // namespace NAlice::NMegamind
