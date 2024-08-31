#pragma once

#include "utils.h"

#include <alice/hollywood/library/hw_service_context/context.h>
#include <alice/hollywood/library/request/request.h>

#include <alice/library/blackbox/blackbox.h>
#include <alice/library/logger/logger.h>
#include <alice/library/scenarios/utils/utils.h>
#include <alice/megamind/protos/scenarios/combinator_request.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <apphost/lib/proto_answers/http.pb.h>

namespace NAlice::NHollywood::NCombinators {

namespace NPrivate {

inline constexpr TStringBuf SCENARIO_ITEM_PREFIX = "scenario_";
inline constexpr TStringBuf PURE_SCENARIO_ITEM_RUN_SUFFIX = "_run_pure_response";
inline constexpr TStringBuf PROXY_SCENARIO_ITEM_RUN_SUFFIX = "_run_http_proxy_response";
inline constexpr TStringBuf PURE_SCENARIO_ITEM_CONTINUE_SUFFIX = "_continue_pure_response";
inline constexpr TStringBuf PROXY_SCENARIO_ITEM_CONTINUE_SUFFIX = "_continue_http_proxy_response";

template <typename TResponseProto>
::google::protobuf::Map<TString, TResponseProto> GetScenarioResponsesFromContext(THwServiceContext& ctx, TStringBuf proxySuffix,
                                                                                 TStringBuf pureSuffix, TStringBuf method,
                                                                                 TRTLogger& logger) {
    ::google::protobuf::Map<TString, TResponseProto> resultResponses;
    const auto itemsFromContext = ctx.GetItemNamesFromContext();
    for (TStringBuf itemName : itemsFromContext) {
        TStringBuf scenarioName = itemName;
        if (scenarioName.SkipPrefix(SCENARIO_ITEM_PREFIX)) {
            if (scenarioName.ChopSuffix(proxySuffix)) {
                auto httpResponse = ctx.GetProtoOrThrow<NAppHostHttp::THttpResponse>(itemName);
                auto response = ParseScenarioResponse<TResponseProto>(httpResponse, method);
                if (response.Error()) {
                    LOG_ERR(logger) << "Cannot parse " << scenarioName << " " << method << " scenario response";
                    continue;
                }
                LOG_INFO(logger) << "Got " << scenarioName << " " << method << " response";
                resultResponses[TString{scenarioName}] = response.Value();
            } else if (scenarioName.ChopSuffix(pureSuffix)) {
                auto response = ctx.GetProtoOrThrow<TResponseProto>(itemName);
                LOG_INFO(logger) << "Got " << scenarioName << " " << method << " response";
                resultResponses[TString{scenarioName}] = response;
            }
        }
    }
    return resultResponses;
}

} // namespace NPrivate

class TCombinatorBaseRequestWrapper {
public:
    explicit TCombinatorBaseRequestWrapper(const NScenarios::TCombinatorRequest_TBaseRequest& request)
        : BaseRequestProto_(request)
        , ExpFlags_{ExpFlagsFromProto(BaseRequestProto_.GetExperiments())}
    {
    }

    const NScenarios::TCombinatorRequest_TBaseRequest& BaseRequestProto() const {
        return BaseRequestProto_;
    }

    const TExpFlags& ExpFlags() const {
        return ExpFlags_;
    }

    TMaybe<TString> ExpFlag(TStringBuf name) const {
        if (const auto* ptr = ExpFlags_.FindPtr(name)) {
            return *ptr;
        }
        return Nothing();
    }

    TMaybe<TString> GetValueFromExpPrefix(const TStringBuf expPrefix) const {
        for (const auto& expFlag : ExpFlags_) {
            if (expFlag.first.StartsWith(expPrefix)) {
                return expFlag.first.substr(expPrefix.size());
            }
        }
        return Nothing();
    }

    template <typename T>
    T LoadValueFromExpPrefix(const TStringBuf expPrefix, const T defaultValue) const {
        T value = defaultValue;
        if (const auto expValue = GetValueFromExpPrefix(expPrefix); expValue.Defined()) {
            TryFromString(*expValue, value);
        }
        return value;
    }

    bool HasExpFlag(TStringBuf name) const {
        const auto maybeFlag = ExpFlag(name);
        return maybeFlag.Defined() && maybeFlag.GetRef() != TStringBuf("0");
    }

    NAlice::NScenarios::TUserPreferences::EFiltrationMode FiltrationMode() const {
        return BaseRequestProto_.GetUserPreferences().GetFiltrationMode();
    }

    const TString& RequestId() const {
        return BaseRequestProto_.GetRequestId();
    }

    const NScenarios::TInterfaces& Interfaces() const {
        return BaseRequestProto_.GetInterfaces();
    }
private:
    const NScenarios::TCombinatorRequest_TBaseRequest& BaseRequestProto_;
    TExpFlags ExpFlags_;
};

class TCombinatorRequestWrapper : public TCombinatorBaseRequestWrapper {
public:
    using TProto = NScenarios::TCombinatorRequest;
public:
    TCombinatorRequestWrapper(const TProto& request,
                                 TRTLogger& logger,
                                 THwServiceContext& ctx)
        : TCombinatorBaseRequestWrapper(request.GetBaseRequest())
        , Proto_(request)
        , Input_(request.GetInput())
        , Logger_(logger)
        , Context(ctx)
    {
        ScenarioRunResponses_ = NPrivate::GetScenarioResponsesFromContext<NScenarios::TScenarioRunResponse>(Context,
            NPrivate::PROXY_SCENARIO_ITEM_RUN_SUFFIX, NPrivate::PURE_SCENARIO_ITEM_RUN_SUFFIX, /* method= */ "run", Logger_);
        ScenarioContinueResponses_ = NPrivate::GetScenarioResponsesFromContext<NScenarios::TScenarioContinueResponse>(Context,
            NPrivate::PROXY_SCENARIO_ITEM_CONTINUE_SUFFIX, NPrivate::PURE_SCENARIO_ITEM_CONTINUE_SUFFIX, /* method= */ "continue", Logger_);
    }

    const TProto& Proto() const {
        return Proto_;
    }

    const ::google::protobuf::Map<TString, NScenarios::TScenarioRunResponse>& GetScenarioRunResponses() const {
        return ScenarioRunResponses_;
    }

    const ::google::protobuf::Map<TString, NScenarios::TScenarioContinueResponse>& GetScenarioContinueResponses() const {
        return ScenarioContinueResponses_;
    }

    const TScenarioInputWrapper& Input() const {
        return Input_;
    }

    const TMaybe<TBlackBoxFullUserInfoProto>& BlackBoxUserInfo() {
        if (!BlackBoxUserInfo_.Defined()) {
            const auto& httpResponse =
                Context.GetMaybeProto<NAppHostHttp::THttpResponse>(BLACKBOX_HTTP_RESPONSE_ITEM);
            if (httpResponse.Defined()) {
                BlackBoxUserInfo_ = ParseBlackBoxHttpResponse(*httpResponse, Logger_);
            }
        }
        return BlackBoxUserInfo_;
    }

private:
    const TProto& Proto_;
    TScenarioInputWrapper Input_;
    TMaybe<TBlackBoxFullUserInfoProto> BlackBoxUserInfo_;
    TRTLogger& Logger_;
    THwServiceContext& Context;
    ::google::protobuf::Map<TString, NScenarios::TScenarioRunResponse> ScenarioRunResponses_;
    ::google::protobuf::Map<TString, NScenarios::TScenarioContinueResponse> ScenarioContinueResponses_;
};

} // NAlice::NHollywood::NCombinators
