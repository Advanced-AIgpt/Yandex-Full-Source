#include "protocol_scenario.h"

#include <alice/megamind/library/apphost_request/node_names.h>
#include <alice/megamind/library/apphost_request/request_builder.h>
#include <alice/megamind/library/experiments/flags.h>

#include <alice/library/analytics/interfaces/analytics_info_builder.h>
#include <alice/library/experiments/experiments.h>
#include <alice/library/metrics/sensors.h>
#include <alice/library/metrics/util.h>
#include <alice/library/proto/proto.h>

#include <util/charset/wide.h>
#include <util/digest/murmur.h>
#include <util/folder/path.h>
#include <util/generic/algorithm.h>
#include <util/stream/file.h>
#include <util/string/builder.h>
#include <util/string/join.h>

namespace NAlice {

using namespace NScenarios;
using namespace NAlice::NMegamind;

namespace {

constexpr TStringBuf RESPONSE_TYPE_COMMIT_CANDIDATE = "commit_candidate";
constexpr TStringBuf RESPONSE_TYPE_ERROR = "error";
constexpr TStringBuf RESPONSE_TYPE_INVALID_RESPONSE = "invalid_response";
constexpr TStringBuf RESPONSE_TYPE_POSTPONED = "postponed";
constexpr TStringBuf RESPONSE_TYPE_PURE = "pure";
constexpr TStringBuf RESPONSE_TYPE_CONTINUE = "post_run";
constexpr TStringBuf RESPONSE_TYPE_SUCCESS = "success";

constexpr TStringBuf METHOD_APPLY = "Apply";
constexpr TStringBuf METHOD_COMMIT = "Commit";
constexpr TStringBuf METHOD_CONTINUE = "Continue";
constexpr TStringBuf METHOD_RUN = "Run";

TScenarioCommitResponse GetDefaultCommitResponse() {
    TScenarioCommitResponse response{};
    response.MutableSuccess()->CopyFrom(TScenarioCommitResponse_TSuccess::default_instance());
    return response;
}

template <typename TProto>
void PutScenarioRequestMetaIntoContext(
    const IContext& ctx,
    const TProto& proto,
    NMegamind::TItemProxyAdapter& itemProxyAdapter,
    const TString& scenarioName,
    const TString& itemName)
{
    TRequestMeta scenarioRequestMeta;
    const auto& baseRequest = proto.GetBaseRequest();

    scenarioRequestMeta.SetContentType(TString{NContentTypes::APPLICATION_PROTOBUF});

    scenarioRequestMeta.SetRandomSeed(baseRequest.GetRandomSeed());

    scenarioRequestMeta.SetLang(baseRequest.GetClientInfo().GetLang());
    scenarioRequestMeta.SetRequestId(baseRequest.GetRequestId());
    if (const auto userLang = static_cast<ELanguage>(baseRequest.GetUserLanguage());
        userLang != ELanguage::LANG_UNK
    ) {
        scenarioRequestMeta.SetUserLang(IsoNameByLanguage(userLang));
    }
    if (const auto& ticket = ctx.Responses().BlackBoxResponse().GetUserTicket(); !ticket.Empty()) {
        scenarioRequestMeta.SetUserTicket(ticket);
    }
    scenarioRequestMeta.SetClientIP(baseRequest.GetOptions().GetClientIP());

    static constexpr TStringBuf oAuthPrefix = "OAuth ";
    const auto& oAuthToken = ctx.AuthToken();
    if (oAuthToken.Defined() && oAuthToken->StartsWith(oAuthPrefix) && ctx.IsOAuthEnabled(scenarioName)) {
        scenarioRequestMeta.SetOAuthToken(oAuthToken->substr(oAuthPrefix.size()));
    }

    itemProxyAdapter.PutIntoContext(scenarioRequestMeta, itemName);
}

} // namespace

namespace NImpl {

TStringBuf GetResponseType(const TScenarioRunResponse& response) {
    switch (response.GetResponseCase()) {
        case TScenarioRunResponse::ResponseCase::kResponseBody:
            return RESPONSE_TYPE_PURE;
        case TScenarioRunResponse::ResponseCase::kCommitCandidate:
            return RESPONSE_TYPE_COMMIT_CANDIDATE;
        case TScenarioRunResponse::ResponseCase::kApplyArguments:
            return RESPONSE_TYPE_POSTPONED;
        case TScenarioRunResponse::ResponseCase::kError:
            return RESPONSE_TYPE_ERROR;
        case TScenarioRunResponse::ResponseCase::kContinueArguments:
            return RESPONSE_TYPE_CONTINUE;
        case TScenarioRunResponse::ResponseCase::RESPONSE_NOT_SET:
            return RESPONSE_TYPE_INVALID_RESPONSE;
    }
}

template <typename TProtoResponse>
TStringBuf GetApplyResponseType(const TProtoResponse& response) {
    switch (response.GetResponseCase()) {
        case TProtoResponse::ResponseCase::kResponseBody:
            return RESPONSE_TYPE_SUCCESS;
        case TProtoResponse::ResponseCase::kError:
            return RESPONSE_TYPE_ERROR;
        case TProtoResponse::ResponseCase::RESPONSE_NOT_SET:
            return RESPONSE_TYPE_INVALID_RESPONSE;
    }
}

TStringBuf GetResponseType(const TScenarioApplyResponse& response) {
    return GetApplyResponseType(response);
}

TStringBuf GetResponseType(const TScenarioContinueResponse& response) {
    return GetApplyResponseType(response);
}

TStringBuf GetResponseType(const TScenarioCommitResponse& response) {
    switch (response.GetResponseCase()) {
        case TScenarioCommitResponse::ResponseCase::kSuccess:
            return RESPONSE_TYPE_SUCCESS;
        case TScenarioCommitResponse::ResponseCase::kError:
            return RESPONSE_TYPE_ERROR;
        case TScenarioCommitResponse::ResponseCase::RESPONSE_NOT_SET:
            return RESPONSE_TYPE_INVALID_RESPONSE;
    }
}

} // namespace NImpl

// TConfigBasedProtocolScenario -----------------------------------------------------------
TConfigBasedProtocolScenario::TConfigBasedProtocolScenario(const TScenarioConfig& config)
    : TProtocolScenario(config.GetName())
    , Config(config)
    , LabelsGenerator(to_upper(config.GetName()))
    , AcceptedFrames(Reserve(Config.GetAcceptedFrames().size()))
{
    for (const auto& frame : Config.GetAcceptedFrames()) {
        AcceptedFrames.push_back(frame);
    }

    for (const auto& lang : Config.GetLanguages()) {
        SupportedLanguages.insert(static_cast<ELanguage>(lang));
    }
}

bool TConfigBasedProtocolScenario::IsEnabled(const IContext& ctx) const {
    if (GetName() == MM_PROTO_VINS_SCENARIO) {
        return Config.GetEnabled() && ctx.IsProtoVinsEnabled();
    }
    return Config.GetEnabled() || ctx.HasExpFlag(EXP_PREFIX_MM_ENABLE_PROTOCOL_SCENARIO + GetName());
}

TVector<TString> TConfigBasedProtocolScenario::GetAcceptedFrames() const {
    return AcceptedFrames;
}

bool TConfigBasedProtocolScenario::AcceptsAnyUtterance() const {
    return Config.GetAcceptsAnyUtterance();
}

bool TConfigBasedProtocolScenario::AcceptsImageInput() const {
    return Config.GetAcceptsImageInput();
}

bool TConfigBasedProtocolScenario::AcceptsMusicInput() const {
    return Config.GetAcceptsMusicInput();
}

bool TConfigBasedProtocolScenario::AlwaysRecieveAllParsedSemanticFrames() const {
    return Config.GetAlwaysRecieveAllParsedSemanticFrames();
}

bool TConfigBasedProtocolScenario::DependsOnWebSearchResult() const {
    return AnyOf(Config.GetDataSources(), [](const auto& dataSource) {
        return AnyOf(WEB_SOURCES, [&dataSource](const auto& x){
            return dataSource.GetType() == x;
        });
    });
}

TVector<EDataSourceType> TConfigBasedProtocolScenario::GetDataSources() const {
    TVector<EDataSourceType> dataSources;
    for (const auto& dataSource : Config.GetDataSources()) {
        dataSources.push_back(dataSource.GetType());
    }
    return dataSources;
}

TVector<EDataSourceType> TConfigBasedProtocolScenario::GetRequiredDataSources() const {
    TVector<EDataSourceType> requiredDataSources;
    for (const auto& dataSource : Config.GetDataSources()) {
        if (dataSource.GetIsRequired()) {
            requiredDataSources.push_back(dataSource.GetType());
        }
    }
    return requiredDataSources;
}

template <typename TResponseProto>
TErrorOr<TResponseProto> TConfigBasedProtocolScenario::ParseResponse(NHttpFetcher::TResponse::TRef response,
                                                                     TStringBuf method, NMetrics::ISensors& sensors) const {
    const auto errorOrProto = NImpl::ParseResponse<TResponseProto>(response, method);
    WriteErrorOrProtoMetrics(errorOrProto, method, sensors);
    return errorOrProto;
}

bool TConfigBasedProtocolScenario::IsLanguageSupported(const ELanguage& language) const {
    return SupportedLanguages.contains(language);
}

template <typename TResponseProto>
void TConfigBasedProtocolScenario::WriteErrorOrProtoMetrics(const TErrorOr<TResponseProto>& errorOrProto,
                                                            TStringBuf method, NMetrics::ISensors& sensors) const {
    TString type;
    if (const auto* e = errorOrProto.Error()) {
        type = TStringBuilder{} << "error_" << ToString(e->Type);
    } else {
        const auto& response = errorOrProto.Value();
        const TString version = response.GetVersion();
        i64 intVersion = 0;
        if (TryFromString<i64>(version, intVersion)) {
            sensors.SetIntGauge(LabelsGenerator.Version(), intVersion);
        } else {
            sensors.SetIntGauge(LabelsGenerator.Version(), MurmurHash<i64>(version.data(), version.size()));
        }
        type = NImpl::GetResponseType(response);
    }
    sensors.IncRate(LabelsGenerator.RequestResponseType(method, type));
}

void TConfigBasedProtocolScenario::WriteSizeMetrics(const NScenarios::TScenarioRunResponse& response, NMetrics::ISensors& sensors) const {
    switch (response.GetResponseCase()) {
        case TScenarioRunResponse::kResponseBody:
            WriteStateSize(response.GetResponseBody().GetState().ByteSizeLong(), sensors);
            break;
        case TScenarioRunResponse::kCommitCandidate:
            WriteArgumentsSize(/* argumentsType= */ TStringBuf("commit_candidate"),
                               response.GetCommitCandidate().GetArguments().ByteSizeLong(),
                               sensors);
            WriteStateSize(response.GetCommitCandidate().GetResponseBody().ByteSizeLong(), sensors);
            break;
        case TScenarioRunResponse::kApplyArguments:
            WriteArgumentsSize(/* argumentsType= */ TStringBuf("apply_arguments"),
                               response.GetApplyArguments().ByteSizeLong(), sensors);
            break;
        case TScenarioRunResponse::kContinueArguments:
            WriteArgumentsSize(/* argumentsType= */ TStringBuf("continue_arguments"),
                               response.GetContinueArguments().ByteSizeLong(), sensors);
            break;
        case TScenarioRunResponse::kError:
            break;
        case TScenarioRunResponse::RESPONSE_NOT_SET:
            break;
    }
}

void TConfigBasedProtocolScenario::WriteArgumentsSize(const TStringBuf argumentsType, const ui64 sizeBytes, NMetrics::ISensors& sensors) const {
    sensors.AddHistogram(LabelsGenerator.ArgumentsSize(argumentsType), sizeBytes, NMetrics::SIZE_INTERVALS);
}

void TConfigBasedProtocolScenario::WriteStateSize(const ui64 sizeBytes, NMetrics::ISensors& sensors) const {
    sensors.AddHistogram(LabelsGenerator.StateSize(), sizeBytes, NMetrics::SIZE_INTERVALS);
}

// TConfigBasedAppHostProxyProtocolScenario -----------------------------------------------------------
TConfigBasedAppHostProxyProtocolScenario::TConfigBasedAppHostProxyProtocolScenario(const TScenarioConfig& config)
    : TConfigBasedProtocolScenario{config}
    , AppHostProxyItemNames{config.GetName(), HTTP_PROXY_REQUEST_SUFFIX, HTTP_PROXY_RESPONSE_SUFFIX}
    , AppHostPureItemNames{config.GetName(), PURE_REQUEST_SUFFIX, PURE_RESPONSE_SUFFIX}
    , UseAppHostPureSceanrioFlag{TStringBuilder{} << "use_app_host_pure_" << Config.GetName() << "_scenario"}
{
}

TStatus TConfigBasedAppHostProxyProtocolScenario::StartRun(const IContext& ctx,
                                                           const TScenarioRunRequest& request,
                                                           NMegamind::TItemProxyAdapter& itemProxyAdapter) const {
    if (Config.GetHandlers().GetIsTransferringToAppHostPure() && ctx.HasExpFlag(UseAppHostPureSceanrioFlag)) {
        itemProxyAdapter.PutIntoContext(request, AppHostPureItemNames.RunRequest);
        // Some of the information can also be found in TScenarioRunRequest,
        // but this allows to reduce the number of network hops in Hollywood
        PutScenarioRequestMetaIntoContext(ctx, request, itemProxyAdapter, Config.GetName(), AppHostPureItemNames.RequestMeta);
        return Success();
    }

    NMegamind::TAppHostHttpProxyMegamindRequestBuilder builder;
    TStatus error = FillRequest(ctx, request, builder, ctx.IsOAuthEnabled(Config.GetName()));
    if (error.Defined()) {
        return *error;
    }
    builder.SetPath("/run");
    itemProxyAdapter.PutIntoContext(builder.CreateRequest(), AppHostProxyItemNames.RunRequest);
    return Success();
}


TErrorOr<TScenarioRunResponse>
TConfigBasedAppHostProxyProtocolScenario::FinishRun(const IContext& ctx, NMegamind::TItemProxyAdapter& itemProxyAdapter) const {
    if (Config.GetHandlers().GetIsTransferringToAppHostPure() && ctx.HasExpFlag(UseAppHostPureSceanrioFlag)) {
        TErrorOr<TScenarioRunResponse> responseProto =
            itemProxyAdapter.GetFromContext<TScenarioRunResponse>(AppHostPureItemNames.RunResponse);
        WriteErrorOrProtoMetrics<TScenarioRunResponse>(responseProto, METHOD_RUN, ctx.Sensors());
        if (responseProto.IsSuccess()) {
            WriteSizeMetrics(responseProto.Value(), ctx.Sensors());
        }
        return responseProto;
    }

    TErrorOr<NAppHostHttp::THttpResponse> httpResponseProto =
        itemProxyAdapter.GetFromContext<NAppHostHttp::THttpResponse>(AppHostProxyItemNames.RunResponse);
    auto response = ParseScenarioResponse<TScenarioRunResponse>(httpResponseProto, METHOD_RUN);
    if (response.IsSuccess()) {
        WriteSizeMetrics(response.Value(), ctx.Sensors());
    }
    WriteErrorOrProtoMetrics<TScenarioRunResponse>(response, METHOD_RUN, ctx.Sensors());
    return response;
}

TErrorOr<TScenarioContinueResponse>
TConfigBasedAppHostProxyProtocolScenario::FinishContinue(const IContext& ctx, NMegamind::TItemProxyAdapter& itemProxyAdapter) const {
    if (Config.GetHandlers().GetIsTransferringToAppHostPure() && ctx.HasExpFlag(UseAppHostPureSceanrioFlag)) {
        TErrorOr<TScenarioContinueResponse> response =
            itemProxyAdapter.GetFromContext<TScenarioContinueResponse>(AppHostPureItemNames.ContinueResponse);
        if (response.IsSuccess()) {
            WriteStateSize(response.Value().GetResponseBody().GetState().ByteSizeLong(), ctx.Sensors());
        }
        WriteErrorOrProtoMetrics<TScenarioContinueResponse>(response, METHOD_CONTINUE, ctx.Sensors());
        return response;
    }

    TErrorOr<NAppHostHttp::THttpResponse> httpResponseProto =
        itemProxyAdapter.GetFromContext<NAppHostHttp::THttpResponse>(AppHostProxyItemNames.ContinueResponse);
    auto response = ParseScenarioResponse<TScenarioContinueResponse>(httpResponseProto, METHOD_CONTINUE);
    if (response.IsSuccess()) {
        WriteStateSize(response.Value().GetResponseBody().GetState().ByteSizeLong(), ctx.Sensors());
    }
    WriteErrorOrProtoMetrics<TScenarioContinueResponse>(response, METHOD_CONTINUE, ctx.Sensors());
    return response;
}

TStatus TConfigBasedAppHostProxyProtocolScenario::StartApply(const IContext& ctx,
                                                             const TScenarioApplyRequest& request,
                                                             NMegamind::TItemProxyAdapter& itemProxyAdapter) const {
    if (Config.GetHandlers().GetIsTransferringToAppHostPure() && ctx.HasExpFlag(UseAppHostPureSceanrioFlag)) {
        itemProxyAdapter.PutIntoContext(request, AppHostPureItemNames.ApplyRequest);
        // Some of the information can also be found in TScenarioRunRequest,
        // but this allows to reduce the number of network hops in Hollywood
        PutScenarioRequestMetaIntoContext(ctx, request, itemProxyAdapter, Config.GetName(), AppHostPureItemNames.RequestMeta);
        return Success();
    }

    NMegamind::TAppHostHttpProxyMegamindRequestBuilder builder;
    TStatus error = FillRequest(ctx, request, builder, ctx.IsOAuthEnabled(Config.GetName()));
    if (error.Defined()) {
        return *error;
    }
    builder.SetPath("/apply");
    itemProxyAdapter.PutIntoContext(builder.CreateRequest(), AppHostProxyItemNames.ApplyRequest);
    return Success();
}

TErrorOr<TScenarioApplyResponse>
TConfigBasedAppHostProxyProtocolScenario::FinishApply(const IContext& ctx, NMegamind::TItemProxyAdapter& itemProxyAdapter) const {
    if (Config.GetHandlers().GetIsTransferringToAppHostPure() && ctx.HasExpFlag(UseAppHostPureSceanrioFlag)) {
        TErrorOr<TScenarioApplyResponse> response =
            itemProxyAdapter.GetFromContext<TScenarioApplyResponse>(AppHostPureItemNames.ApplyResponse);
        if (response.IsSuccess()) {
            WriteStateSize(response.Value().GetResponseBody().GetState().ByteSizeLong(), ctx.Sensors());
        }
        WriteErrorOrProtoMetrics<TScenarioApplyResponse>(response, METHOD_APPLY, ctx.Sensors());
        return response;
    }

    TErrorOr<NAppHostHttp::THttpResponse> httpResponseProto =
        itemProxyAdapter.GetFromContext<NAppHostHttp::THttpResponse>(AppHostProxyItemNames.ApplyResponse);
    auto response = ParseScenarioResponse<TScenarioApplyResponse>(httpResponseProto, METHOD_APPLY);
    if (response.IsSuccess()) {
        WriteStateSize(response.Value().GetResponseBody().GetState().ByteSizeLong(), ctx.Sensors());
    }
    WriteErrorOrProtoMetrics<TScenarioApplyResponse>(response, METHOD_APPLY, ctx.Sensors());
    return response;
}

TStatus TConfigBasedAppHostProxyProtocolScenario::StartCommit(const IContext& ctx,
                                                              const TScenarioApplyRequest& request,
                                                              NMegamind::TItemProxyAdapter& itemProxyAdapter) const {
    if (ctx.HasExpFlag(EXP_DONT_CALL_SCENARIO_COMMIT)) {
        LOG_INFO(ctx.Logger()) << "Skipping scenario commit stage because " << EXP_DONT_CALL_SCENARIO_COMMIT
                               << " has been found, success is going to be returned";
        return Success();
    }

    if (Config.GetHandlers().GetIsTransferringToAppHostPure() && ctx.HasExpFlag(UseAppHostPureSceanrioFlag)) {
        itemProxyAdapter.PutIntoContext(request, AppHostPureItemNames.CommitRequest);
        // Some of the information can also be found in TScenarioRunRequest,
        // but this allows to reduce the number of network hops in Hollywood
        PutScenarioRequestMetaIntoContext(ctx, request, itemProxyAdapter, Config.GetName(), AppHostPureItemNames.RequestMeta);
        return Success();
    }

    NMegamind::TAppHostHttpProxyMegamindRequestBuilder builder;
    TStatus error = FillRequest(ctx, request, builder, ctx.IsOAuthEnabled(Config.GetName()));
    if (error.Defined()) {
        return *error;
    }
    builder.SetPath("/commit");
    itemProxyAdapter.PutIntoContext(builder.CreateRequest(), AppHostProxyItemNames.CommitRequest);
    return Success();
}

TErrorOr<TScenarioCommitResponse>
TConfigBasedAppHostProxyProtocolScenario::FinishCommit(const IContext& ctx, NMegamind::TItemProxyAdapter& itemProxyAdapter) const {
    if (ctx.HasExpFlag(EXP_DONT_CALL_SCENARIO_COMMIT)) {
        LOG_INFO(ctx.Logger()) << "Skipping scenario commit stage because " << EXP_DONT_CALL_SCENARIO_COMMIT
                               << " has been found, success is going to be returned";
        return GetDefaultCommitResponse();
    }

    if (Config.GetHandlers().GetIsTransferringToAppHostPure() && ctx.HasExpFlag(UseAppHostPureSceanrioFlag)) {
        return itemProxyAdapter.GetFromContext<TScenarioCommitResponse>(AppHostPureItemNames.CommitResponse);
    }

    TErrorOr<NAppHostHttp::THttpResponse> httpResponseProto =
        itemProxyAdapter.GetFromContext<NAppHostHttp::THttpResponse>(AppHostProxyItemNames.CommitResponse);
    auto response = ParseScenarioResponse<TScenarioCommitResponse>(httpResponseProto, METHOD_COMMIT);
    WriteErrorOrProtoMetrics<TScenarioCommitResponse>(response, METHOD_COMMIT, ctx.Sensors());
    return response;
}

bool TConfigBasedAppHostProxyProtocolScenario::PassDataSourcesInContext(const IContext& ctx) const {
    return !(ctx.HasExpFlag(UseAppHostPureSceanrioFlag) && Config.GetHandlers().GetIsTransferringToAppHostPure());
}

// TConfigBasedAppHostPureProtocolScenario -----------------------------------------------------------
TConfigBasedAppHostPureProtocolScenario::TConfigBasedAppHostPureProtocolScenario(const TScenarioConfig& config)
    : TConfigBasedProtocolScenario{config}
    , AppHostPureItemNames{config.GetName(), PURE_REQUEST_SUFFIX, PURE_RESPONSE_SUFFIX}
{
}

TStatus TConfigBasedAppHostPureProtocolScenario::StartRun(const IContext& ctx,
                                                          const TScenarioRunRequest& request,
                                                          NMegamind::TItemProxyAdapter& itemProxyAdapter) const {
    itemProxyAdapter.PutIntoContext(request, AppHostPureItemNames.RunRequest);

    // Some of the information can also be found in TScenarioRunRequest,
    // but this allows to reduce the number of network hops in Hollywood
    PutScenarioRequestMetaIntoContext(ctx, request, itemProxyAdapter, Config.GetName(), AppHostPureItemNames.RequestMeta);
    return Success();
}


TErrorOr<TScenarioRunResponse>
TConfigBasedAppHostPureProtocolScenario::FinishRun(const IContext& ctx, NMegamind::TItemProxyAdapter& itemProxyAdapter) const {
    TErrorOr<TScenarioRunResponse> responseProto =
        itemProxyAdapter.GetFromContext<TScenarioRunResponse>(AppHostPureItemNames.RunResponse);
    WriteErrorOrProtoMetrics<TScenarioRunResponse>(responseProto, METHOD_RUN, ctx.Sensors());
    if (responseProto.IsSuccess()) {
        WriteSizeMetrics(responseProto.Value(), ctx.Sensors());
    }
    return responseProto;
}

TErrorOr<TScenarioContinueResponse>
TConfigBasedAppHostPureProtocolScenario::FinishContinue(const IContext& ctx, NMegamind::TItemProxyAdapter& itemProxyAdapter) const {
    TErrorOr<TScenarioContinueResponse> response =
        itemProxyAdapter.GetFromContext<TScenarioContinueResponse>(AppHostPureItemNames.ContinueResponse);
    if (response.IsSuccess()) {
        WriteStateSize(response.Value().GetResponseBody().GetState().ByteSizeLong(), ctx.Sensors());
    }
    WriteErrorOrProtoMetrics<TScenarioContinueResponse>(response, METHOD_CONTINUE, ctx.Sensors());
    return response;
}

TStatus TConfigBasedAppHostPureProtocolScenario::StartApply(const IContext& ctx,
                                                            const TScenarioApplyRequest& request,
                                                            NMegamind::TItemProxyAdapter& itemProxyAdapter) const {
    itemProxyAdapter.PutIntoContext(request, AppHostPureItemNames.ApplyRequest);

    // Some of the information can also be found in TScenarioRunRequest,
    // but this allows to reduce the number of network hops in Hollywood
    PutScenarioRequestMetaIntoContext(ctx, request, itemProxyAdapter, Config.GetName(), AppHostPureItemNames.RequestMeta);
    return Success();
}

TErrorOr<TScenarioApplyResponse>
TConfigBasedAppHostPureProtocolScenario::FinishApply(const IContext& ctx, NMegamind::TItemProxyAdapter& itemProxyAdapter) const {
    TErrorOr<TScenarioApplyResponse> response =
        itemProxyAdapter.GetFromContext<TScenarioApplyResponse>(AppHostPureItemNames.ApplyResponse);
    if (response.IsSuccess()) {
        WriteStateSize(response.Value().GetResponseBody().GetState().ByteSizeLong(), ctx.Sensors());
    }
    WriteErrorOrProtoMetrics<TScenarioApplyResponse>(response, METHOD_APPLY, ctx.Sensors());
    return response;
}

TStatus TConfigBasedAppHostPureProtocolScenario::StartCommit(const IContext& ctx,
                                                             const TScenarioApplyRequest& request,
                                                             NMegamind::TItemProxyAdapter& itemProxyAdapter) const {
    if (ctx.HasExpFlag(EXP_DONT_CALL_SCENARIO_COMMIT)) {
        LOG_INFO(ctx.Logger()) << "Skipping scenario commit stage because " << EXP_DONT_CALL_SCENARIO_COMMIT
                               << " has been found, success is going to be returned";
        return Success();
    }

    itemProxyAdapter.PutIntoContext(request, AppHostPureItemNames.CommitRequest);

    // Some of the information can also be found in TScenarioRunRequest,
    // but this allows to reduce the number of network hops in Hollywood
    PutScenarioRequestMetaIntoContext(ctx, request, itemProxyAdapter, Config.GetName(), AppHostPureItemNames.RequestMeta);
    return Success();
}

TErrorOr<TScenarioCommitResponse>
TConfigBasedAppHostPureProtocolScenario::FinishCommit(const IContext& ctx, NMegamind::TItemProxyAdapter& itemProxyAdapter) const {
    if (ctx.HasExpFlag(EXP_DONT_CALL_SCENARIO_COMMIT)) {
        LOG_INFO(ctx.Logger()) << "Skipping scenario commit stage because " << EXP_DONT_CALL_SCENARIO_COMMIT
                               << " has been found, success is going to be returned";
        return GetDefaultCommitResponse();
    }

    auto response = itemProxyAdapter.GetFromContext<TScenarioCommitResponse>(AppHostPureItemNames.CommitResponse);
    WriteErrorOrProtoMetrics<TScenarioCommitResponse>(response, METHOD_COMMIT, ctx.Sensors());
    return response;
}

} // namespace NAlice
