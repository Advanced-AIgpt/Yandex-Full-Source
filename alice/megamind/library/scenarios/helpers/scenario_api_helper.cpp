#include "scenario_api_helper.h"

#include <alice/megamind/library/scenarios/helpers/get_request_language/get_scenario_request_language.h>
#include <alice/megamind/library/scenarios/interface/blackbox.h>
#include <alice/megamind/library/scenarios/protocol/protocol_scenario.h>
#include <alice/megamind/library/scenarios/utils/markup_converter.h>

#include <alice/megamind/library/context/context.h>
#include <alice/megamind/library/experiments/flags.h>
#include <alice/megamind/library/models/directives/callback_directive_model.h>
#include <alice/megamind/library/serializers/scenario_proto_serializer.h>
#include <alice/megamind/library/stack_engine/stack_engine.h>
#include <alice/megamind/library/worldwide/language/is_alice_worldwide_language.h>

#include <alice/megamind/protos/common/gc_memory_state.pb.h>
#include <alice/megamind/protos/scenarios/request.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <alice/protos/data/language/language.pb.h>

#include <alice/library/client/client_features.h>
#include <alice/library/experiments/experiments.h>
#include <alice/library/experiments/utils.h>
#include <alice/library/geo/protos/user_location.pb.h>
#include <alice/library/json/json.h>

#include <google/protobuf/struct.pb.h>
#include <google/protobuf/util/json_util.h>

#include <kernel/alice/begemot_nlu_features/access.h>

#include <algorithm>

using namespace ::google::protobuf;

namespace NAlice::NMegamind {

using TScenarioRunRequest = NScenarios::TScenarioRunRequest;
using TScenarioApplyRequest = NScenarios::TScenarioApplyRequest;

namespace {

    void AddFakeBestAsrResult(::google::protobuf::RepeatedPtrField<TAsrResult>& asrData, const TString& utterance) {
        TAsrResult fakeAsrResult;
        fakeAsrResult.SetUtterance(utterance);
        fakeAsrResult.SetNormalized(utterance);
        if (const auto& oldBestAsrResult = asrData.at(0); oldBestAsrResult.HasConfidence()) {
            fakeAsrResult.SetConfidence(oldBestAsrResult.GetConfidence());
        }

        asrData.Add();
        std::move_backward(asrData.begin(), asrData.end() - 1, asrData.end());
        *asrData.begin() = std::move(fakeAsrResult);
    }

} // namespace

namespace NImpl {

THashMap<::ru::yandex::alice::memento::proto::EConfigKey, const FieldDescriptor*> LoadUserConfigsMapping() {
    THashMap<::ru::yandex::alice::memento::proto::EConfigKey, const FieldDescriptor*> mapping{};
    const auto& descriptor = *TUserConfigs::descriptor();
    for (int i = 0; i < descriptor.field_count(); ++i) {
        const auto& field = descriptor.field(i);
        mapping[field->options().GetExtension(::ru::yandex::alice::memento::proto::key)] = field;
    }
    return mapping;
}

void ConstructMementoUserConfigs(TUserConfigs& userConfigs, const TScenarioConfig& config,
                                 const TUserConfigs& sourceUserConfigs) {
    static const auto mapping = LoadUserConfigsMapping();
    const auto& reflection = *sourceUserConfigs.GetReflection();
    for (const auto& param : config.GetMementoUserConfigs()) {
        if (!mapping.contains(param.GetConfigKey())) {
            continue;
        }
        const auto* field = mapping.at(param.GetConfigKey());
        if (!reflection.HasField(sourceUserConfigs, field)) {
            continue;
        }
        reflection.MutableMessage(&userConfigs, field)->CopyFrom(reflection.GetMessage(sourceUserConfigs, field));
    }
}

void AddScenarioDataSourcesByExp(const TString& scenarioName, const IContext::TExpFlags& expFlags, TVector<EDataSourceType>& scenarioDataSources) {
    if (const auto dataSourcesByExpMaybe = GetExperimentValueWithPrefix(expFlags, EXP_SET_SCENARIO_DATASOURCE_PREFIX + scenarioName + ":")) {
        for (const auto& it : StringSplitter(dataSourcesByExpMaybe.GetRef()).Split(':')) {
            EDataSourceType dataSourceType;
            if (EDataSourceType_Parse(ToString(it.Token()), &dataSourceType)) {
                scenarioDataSources.push_back(dataSourceType);
            }
        }
    }
}

bool DependsOnWebSearchResultByExp(const TString& scenarioName, const IContext::TExpFlags& expFlags) {
    const auto dataSourcesByExpMaybe = GetExperimentValueWithPrefix(expFlags, EXP_SET_SCENARIO_DATASOURCE_PREFIX + scenarioName + ":");
    if (!dataSourcesByExpMaybe.Defined()) {
        return false;
    }

    for (const auto& it : StringSplitter(dataSourcesByExpMaybe.GetRef()).Split(':')) {
        EDataSourceType dataSourceType;
        if (!EDataSourceType_Parse(ToString(it.Token()), &dataSourceType)) {
            continue;
        }
        if (FindPtr(WEB_SOURCES, dataSourceType)) {
            return true;
        }
    }

    return false;
}

void ConstructNluFeatures(
    ::google::protobuf::RepeatedPtrField<NScenarios::TNluFeature>& featuresProto,
    const TScenarioConfig& config, const TWizardResponse& wizardResponse)
{
    if (!wizardResponse.GetProtoResponse().HasAliceNluFeatures()) {
        return;
    }

    const auto& featureContainer = wizardResponse.GetProtoResponse().GetAliceNluFeatures().GetFeatureContainer();

    for (const TNluFeatureParams& featureRequest : config.GetNluFeatures()) {
        const NNluFeatures::ENluFeature feature = featureRequest.GetFeature();
        const TMaybe<float> featureValue = NNluFeatures::GetFeatureMaybe(featureContainer, feature);

        if (featureValue.Defined()) {
            NScenarios::TNluFeature result;
            result.SetFeature(feature);
            result.SetValue(featureValue.GetRef());
            featuresProto.Add(std::move(result));
        }
    }
}

struct TPolyglotSpecificData {
    ELanguage Language;
    TMaybe<TString> NormalizedUtterance;
    TMaybe<TString> TranslatedUtterance;
};

TPolyglotSpecificData SelectPolyglotSpecificData(const TScenarioConfig& config, const IEvent& event, const IContext& ctx) {
    const auto language = GetScenarioRequestLanguage(config, ctx);

    if (language == ctx.Language()) {
        return TPolyglotSpecificData{
            .Language = language,
            .NormalizedUtterance = ctx.NormalizedPolyglotUtterance(),
            .TranslatedUtterance = Nothing(),
        };
    }

    TMaybe<TString> translatedUtterance = Nothing();
    if (event.GetUtterance() && event.GetUtterance() == ctx.PolyglotUtterance()) {
        translatedUtterance = ctx.TranslatedUtterance();
    }
    return TPolyglotSpecificData{
        .Language = language,
        .NormalizedUtterance = ctx.NormalizedTranslatedUtterance(),
        .TranslatedUtterance = translatedUtterance,
    };
}

void ConstructBaseRequest(
    NScenarios::TScenarioBaseRequest* request,
    const TScenarioConfig& config,
    const TSpeechKitRequest& skRequest,
    ui64 seed,
    ELanguage userLanguage,
    TScenarioSessionView sessionView,
    TMementoDataView mementoDataView,
    const TRequest& requestModel,
    const TWizardResponse& wizardResponse)
{
    const auto& skRequestHeader = skRequest->GetHeader();
    const auto& additionalOptions = skRequest->GetRequest().GetAdditionalOptions();
    request->SetRequestId(skRequestHeader.GetRequestId());
    if (requestModel.GetScenarioDialogId().Defined()) {
        request->SetDialogId(*requestModel.GetScenarioDialogId());
    }
    request->SetServerTimeMs(additionalOptions.GetServerTimeMs());
    request->SetRandomSeed(seed);
    request->SetUserLanguage(static_cast<ELang>(userLanguage));
    *request->MutableClientInfo() = skRequest->GetApplication();
    request->MutableClientInfo()->SetTimezone(requestModel.GetUserLocation().UserTimeZone());

    if (const auto& loc = requestModel.GetLocation(); loc.Defined()) {
        auto& location = *request->MutableLocation();
        location.SetLat(loc->GetLatitude());
        location.SetLon(loc->GetLongitude());
        location.SetAccuracy(loc->GetAccuracy());
        location.SetRecency(loc->GetRecency());
        location.SetSpeed(loc->GetSpeed());
    }

    // TODO(jock): remove after QUASAR-1287
    if (const auto& deviceId = skRequest->GetRequest().GetDeviceState().GetDeviceId(); !deviceId.empty()) {
        *request->MutableClientInfo()->MutableDeviceId() = deviceId;
    }

    request->MutableInterfaces()->CopyFrom(requestModel.GetInterfaces());
    request->MutableOptions()->CopyFrom(requestModel.GetOptions());
    request->MutableUserPreferences()->CopyFrom(requestModel.GetUserPreferences());
    request->MutableUserClassification()->CopyFrom(requestModel.GetUserClassification());

    request->SetIsNewSession(sessionView.IsNewSession);
    *request->MutableState() = sessionView.ScenarioState.GetState();
    request->SetIsSessionReset(sessionView.IsSessionReset);

    const auto stackEngine = TStackEngine{requestModel.GetStackEngineCore()};
    request->SetIsStackOwner(stackEngine.IsStackOwner(config.GetName()));

    TExpFlagsToStructVisitor{*request->MutableExperiments()}.Visit(skRequest->GetRequest().GetExperiments());

    auto* memento = request->MutableMemento();
    ConstructMementoUserConfigs(*memento->MutableUserConfigs(), config, mementoDataView.GetUserConfigs());
    if (const auto* scenarioData = mementoDataView.GetScenarioData(); scenarioData) {
        memento->MutableScenarioData()->CopyFrom(*scenarioData);
    }
    const bool enableMementoSurfaceData =
        MapFindPtr(skRequest->GetRequest().GetExperiments().GetStorage(), TString{EXP_ENABLE_MEMENTO_SURFACE_DATA});
    if (const auto* surfaceScenarioData = mementoDataView.GetSurfaceScenarioData();
        enableMementoSurfaceData && surfaceScenarioData)
    {
        memento->MutableSurfaceScenarioData()->CopyFrom(*surfaceScenarioData);
    }

    request->SetRequestSource(requestModel.GetRequestSource());

    if (const auto& origin = requestModel.GetOrigin(); origin.Defined()) {
        request->MutableOrigin()->CopyFrom(*origin);
    }

    ConstructNluFeatures(*request->MutableNluFeatures(), config, wizardResponse);
}

void ConstructInput(
        NScenarios::TInput* input,
        TArrayRef<const TSemanticFrame> semanticFrames,
        const IEvent& event,
        const TMaybe<TString>& normalizedUtterance,
        const TMaybe<TString>& translatedUtterance)
{
    for(auto& frame: semanticFrames) {
        *input->MutableSemanticFrames()->Add() = frame;
    }

    event.FillScenarioInput(normalizedUtterance, input);
    if (translatedUtterance.Defined()) {
        if (input->GetText().GetRawUtterance()) {
            input->MutableText()->SetRawUtterance(translatedUtterance.GetRef());
        }
        if (!input->GetVoice().GetAsrData().empty()) {
            AddFakeBestAsrResult(*input->MutableVoice()->MutableAsrData(), translatedUtterance.GetRef());
        }
    }
}

void ConstructDataSources(
    ::google::protobuf::Map<::google::protobuf::int32, NScenarios::TDataSource>* dataSourcesProto,
    const TScenarioConfig& config,
    NMegamind::IDataSources& dataSources,
    IContext::TExpFlags expFlags,
    const bool dialogHistoryAllowed,
    const bool passDataSourcesInRequest)
{
    TVector<EDataSourceType> scenarioDataSources;
    for (const auto& dataSource : config.GetDataSources()) {
        scenarioDataSources.push_back(dataSource.GetType());
    }
    AddScenarioDataSourcesByExp(config.GetName(), expFlags, scenarioDataSources);

    for (const auto& dataSourceType : scenarioDataSources) {
        const auto& dataSource = dataSources.GetDataSource(dataSourceType);
        if (dataSourceType == EDataSourceType::DIALOG_HISTORY && !dialogHistoryAllowed) {
            continue;
        }
        if (passDataSourcesInRequest && IsDataSourceFilled(dataSource)) {
            (*dataSourcesProto)[dataSourceType].CopyFrom(dataSource);
        }
    }
}

TScenarioRunRequest ConstructRunRequest(
    const TScenarioConfig& config,
    const TSpeechKitRequest& skRequest,
    const IContext& ctx,
    NMegamind::IDataSources& dataSources,
    ui64 seed,
    ELanguage userLanguage,
    TScenarioSessionView sessionView,
    TMementoDataView mementoDataView,
    TArrayRef<const TSemanticFrame> semanticFrames,
    const TRequest& requestModel,
    const TMaybe<TString>& normalizedUtterance,
    const TMaybe<TString>& translatedUtterance,
    TRTLogger& logger,
    const bool passDataSourcesInRequest)
{
    TScenarioRunRequest request;
    ConstructBaseRequest(request.MutableBaseRequest(), config, skRequest, seed, userLanguage, sessionView,
                         mementoDataView, requestModel, ctx.Responses().WizardResponse());
    ConstructInput(request.MutableInput(), semanticFrames, requestModel.GetEvent(), normalizedUtterance, translatedUtterance);
    LOG_WITH_TYPE(logger, TLOG_DEBUG, ELogMessageType::MegamindPrepareScenarioRunRequests)
        << "Constructed RunRequest for "
        << config.GetName()
        << ": (logging without DataSources and DeviceState) "
        << request;
    ConstructDeviceState(*request.MutableBaseRequest()->MutableDeviceState(), config,
                         skRequest->GetRequest().GetDeviceState());
    const bool dialogHistoryAllowed = ctx.ScenarioConfig(config.GetName()).GetDialogManagerParams().GetDialogHistoryAllowed();
    ConstructDataSources(request.MutableDataSources(), config, dataSources, ctx.ExpFlags(), dialogHistoryAllowed,
                         passDataSourcesInRequest);

    return request;
}

TScenarioApplyRequest ConstructApplyRequest(
    const TScenarioConfig& config,
    const TSpeechKitRequest& skRequest,
    ui64 seed,
    ELanguage userLanguage,
    TScenarioSessionView sessionView,
    TMementoDataView mementoDataView,
    TArrayRef<const TSemanticFrame> semanticFrames,
    const google::protobuf::Any& arguments,
    const TRequest& requestModel,
    const TMaybe<TString>& normalizedUtterance,
    const TMaybe<TString>& translatedUtterance,
    const ISession* session,
    const TWizardResponse& wizardResponse)
{
    TScenarioApplyRequest request;
    ConstructBaseRequest(request.MutableBaseRequest(), config, skRequest, seed, userLanguage, sessionView,
                         mementoDataView, requestModel, wizardResponse);
    ConstructDeviceState(*request.MutableBaseRequest()->MutableDeviceState(), config,
                         skRequest->GetRequest().GetDeviceState());
    if (const auto input = session ? session->GetInput() : Nothing(); !input.Defined()) {
        ConstructInput(request.MutableInput(), semanticFrames, requestModel.GetEvent(), normalizedUtterance, translatedUtterance);
    } else {
        *request.MutableInput() = *input;
    }

    *request.MutableArguments() = arguments;
    return request;
}

void ConstructDeviceState(TDeviceState& deviceState, const TScenarioConfig& scenarioConfig,
                          const TDeviceState& sourceDeviceState) {
    if (AnyOf(scenarioConfig.GetDataSources(),
               [](const auto& dataSource) { return IsDeviceStateDataSource(dataSource.GetType()); }))
    {
        return;
    }
    deviceState.CopyFrom(sourceDeviceState);
    // TODO(atsepeleva): remove after ALICE-7246
    if (!sourceDeviceState.GetVideo().HasViewState() && sourceDeviceState.GetVideo().HasPageState()) {
        deviceState.MutableVideo()->MutableViewState()->CopyFrom(sourceDeviceState.GetVideo().GetPageState());
    }
}

} // namespace NAlice::NMegamind::NImpl

TScenarioRunRequest ConstructRunRequest(
    const TScenarioConfig& config,
    const TRequest& requestModel,
    const IContext& ctx,
    NMegamind::IDataSources& dataSources,
    TScenarioSessionView sessionView,
    TMementoDataView mementoDataView,
    TArrayRef<const TSemanticFrame> frames,
    const bool passDataSourcesInRequest)
{
    const auto polyglotSpecificData = NImpl::SelectPolyglotSpecificData(config, requestModel.GetEvent(), ctx);
    return NImpl::ConstructRunRequest(
        config,
        ctx.SpeechKitRequest(),
        ctx,
        dataSources,
        ctx.GetSeed(),
        polyglotSpecificData.Language,
        sessionView,
        mementoDataView,
        frames,
        requestModel,
        polyglotSpecificData.NormalizedUtterance,
        polyglotSpecificData.TranslatedUtterance,
        ctx.Logger(),
        passDataSourcesInRequest);
}

TScenarioApplyRequest ConstructApplyRequest(
    const TScenarioConfig& config,
    const TRequest& requestModel,
    const IContext& ctx,
    TScenarioSessionView sessionView,
    TMementoDataView mementoDataView,
    TArrayRef<const TSemanticFrame> frames,
    const google::protobuf::Any& arguments)
{
    const auto polyglotSpecificData = NImpl::SelectPolyglotSpecificData(config, requestModel.GetEvent(), ctx);
    return NImpl::ConstructApplyRequest(
        config,
        ctx.SpeechKitRequest(),
        ctx.GetSeed(),
        polyglotSpecificData.Language,
        sessionView,
        mementoDataView,
        frames,
        arguments,
        requestModel,
        polyglotSpecificData.NormalizedUtterance,
        polyglotSpecificData.TranslatedUtterance,
        ctx.Session(),
        ctx.Responses().WizardResponse());
}

} // namespace NAlice::NMegamind
