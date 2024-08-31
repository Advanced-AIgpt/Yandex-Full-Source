#include "request.h"

#include <alice/bass/libs/push_notification/handlers/entitysearch_push.h>
#include <alice/bass/libs/push_notification/handlers/simple_push.h>
#include <alice/bass/libs/push_notification/handlers/taxi_push.h>
#include <alice/bass/libs/push_notification/handlers/music_push.h>
#include <alice/bass/libs/push_notification/handlers/onboarding_push.h>
#include <alice/bass/libs/push_notification/handlers/quasar/quasar_pushes.h>
#include <alice/bass/libs/push_notification/handlers/web_search_push.h>

#include <alice/bass/libs/config/config.h>
#include <alice/bass/libs/globalctx/globalctx.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/source_request/source_request.h>

#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/string/builder.h>
#include <library/cpp/cgiparam/cgiparam.h>

namespace NBASS::NPushNotification {
namespace {

const TString SUP_NAME = "Sup";
const TString XIVA_NAME = "Xiva";

TResultValue AppendSupPush(IGlobalContext& globalCtx, const THandler& service, const TString& clientId, TRequests& requests) {
    TSourcesRequestFactory requestFactory(globalCtx.Sources(), globalCtx.Config());
    THolder<NHttpFetcher::TRequest> req = requestFactory.SupProvider().Request();

    NSc::TValue body;
    TStringBuilder receiver;

    if (clientId == service.GetClientInfo().Name) {
        if (service.GetUUId().empty()) {
            return TError{TError::EType::SYSTEM, "UUId is not initialized"};
        }

        receiver << "uuid: " << service.GetUUId();
    } else {
        if (service.GetUId().empty()) {
            return TError{TError::EType::SYSTEM, "UId is not initialized"};
        }

        receiver << "tag:uid=='" << service.GetUId() << "' AND app_id=='" << clientId << '\'';
    }

    body["receiver"].SetArray().Push().SetString(receiver);
    body["ttl"].SetIntNumber(service.GetTTL());

    NSc::TValue& notification = body["notification"];
    notification["body"].SetString(service.GetBody());
    notification["link"].SetString(service.GetUrl());
    notification["title"].SetString(service.GetTitle());
    notification["icon"].SetString(TStringBuf("https://yastatic.net/s3/home/apisearch/alice_icon.png"));
    notification["iconId"].SetString(TStringBuf("2"));
    body["data"]["tag"].SetString(service.GetTag());
    body["data"]["push_id"].SetString(service.GetTag());
    body["project"].SetString("bass");

    if (service.GetThrottlePolicy()) {
        NSc::TValue& throttlePolicy = body["throttle_policies"];
        throttlePolicy["install_id"].SetString(service.GetThrottlePolicy());
        throttlePolicy["device_id"].SetString(service.GetThrottlePolicy());
    }

    req->SetBody(body.ToJson(), TStringBuf("POST"));
    req->SetContentType(TStringBuf("application/json;charset=UTF-8"));
    req->AddHeader("Authorization", TStringBuilder{} << "OAuth " << globalCtx.Config().PushHandler().SupProvider().Token());

    requests.emplace_back(SUP_NAME, std::move(req));
    return ResultSuccess();
}

TResultValue AppendXivaPush(IGlobalContext& globalCtx, const THandler& service, TRequests& requests) {
    TSourcesRequestFactory requestFactory(globalCtx.Sources(), globalCtx.Config());
    THolder<NHttpFetcher::TRequest> req = requestFactory.XivaProvider().Request();

    if (service.GetUId().empty()) {
        return TError{TError::EType::BADREQUEST, "User ID (UId) is not initialized"};
    }

    req->AddCgiParam("user", service.GetUId());
    req->AddCgiParam("event", service.GetQuasarEvent());
    req->AddCgiParam("ttl", ToString(service.GetTTL()));

    NSc::TValue body;
    body["payload"].SetString(service.GetQuasarPayload());
    body["subscriptions"].SetArray().Push()["session"].Push().SetString(service.GetDId());

    req->SetBody(body.ToJson(), TStringBuf("POST"));
    req->SetContentType(TStringBuf("application/json"));
    req->AddHeader("Authorization", TStringBuilder{} << "Xiva " << globalCtx.Config().PushHandler().XivaProvider().Token());

    requests.emplace_back(XIVA_NAME, std::move(req));
    return ResultSuccess();
}
} // namespace

TResult GetRequests(IGlobalContext& globalCtx, TStringBuf request) {
    const NSc::TValue requestJson = NSc::TValue::FromJsonThrow(request);
    TApiSchemeHolder scheme{requestJson};
    const NSc::TValue callbackDataJson = NSc::TValue::FromJsonThrow(scheme->CallbackData().Get());
    TCallbackDataSchemeHolder callbackData{callbackDataJson};

    TStringBuilder errMsg;
    auto onError = [&errMsg](TStringBuf path, TStringBuf msg) {
        if (errMsg) {
            errMsg << TStringBuf("; ");
        }
        errMsg << path << TStringBuf(": ") << msg;
    };
    if (!scheme->Validate({}, true /* strict */, onError))
        return TError{TError::EType::BADREQUEST, TStringBuilder{} << "JsonSchemeValidation: " << errMsg};

    if (!callbackData->Validate({}, true /* strict */, onError))
        return TError{TError::EType::BADREQUEST, TStringBuilder{} << "CallbackData JsonSchemeValidation: " << errMsg};

    return GetRequests(globalCtx, scheme, callbackData);
}

TResult GetRequestsLocal(IGlobalContext& globalCtx, NSc::TValue serviceData, TString service, TString event, TCallbackDataSchemeHolder callbackData) {
    TApiSchemeHolder scheme{};
    scheme->Event() = std::move(event);
    scheme->Service() = std::move(service);
    scheme->CallbackData() = "";
    *scheme->ServiceData().GetMutable() = serviceData;
    return GetRequests(globalCtx, scheme, callbackData);
}

TResult GetRequests(IGlobalContext& globalCtx, TApiSchemeHolder scheme, TCallbackDataSchemeHolder callbackData) {
    THashMap<TString, TSimpleSharedPtr<IHandlerGenerator>> handlersFactory{
        // {"simple_push", MakeSimpleShared<TSimplePush>()}, // example
        {"taxi", MakeSimpleShared<TTaxiPush>()},
        {"music", MakeSimpleShared<TMusicPush>()},
        {"web_search", MakeSimpleShared<TWebSearchPush>()},
        {"quasar", MakeSimpleShared<TQuasarPushes>()},
        {"onboarding", MakeSimpleShared<TOnboardingPush>()},
        {"entity_search_push", MakeSimpleShared<TEntitySearchPush>()},
    };

    const auto* factory = handlersFactory.FindPtr(TString{*scheme->Service()});
    if (!factory) {
        return TError{TError::EType::INVALIDPARAM, TStringBuilder{} << "no handler found for '" << scheme->Service() << '\''};
    }

    THandler handler(callbackData.Scheme());
    if (TResultValue error = (*factory)->Generate(handler, scheme); error) {
        return std::move(*error);
    }
    if (handler.GetPushes().empty()) {
        return TError{TError::EType::INVALIDPARAM,
                      TStringBuilder{} << "no handler found for '" << scheme->Service() << "' and event '" << scheme->Event() << '\''
        };
    }

    TResultValue error;
    TRequests requests(Reserve(handler.GetPushes().size() /* number of pushes */));
    for (const auto& clientId : handler.GetPushes()) {
        TClientInfo clientInfo(clientId);

        if (clientInfo.IsSmartSpeaker()) {
            error = AppendXivaPush(globalCtx, handler, requests);
        } else {
            error = AppendSupPush(globalCtx, handler, clientId, requests);
        }

        if (error) {
            return std::move(*error);
        }
    }
    return requests;
}

void SendPushUnsafe(const NPushNotification::TResult& requestsVariant, TStringBuf event) {
    if (auto requests = std::get_if<NPushNotification::TRequests>(&requestsVariant)) {
        TVector<NHttpFetcher::THandle::TRef> fetchingRequests;
        for (const auto& req : *requests) {
            fetchingRequests.push_back(req.Request->Fetch());
            LOG(INFO) << "Push service request: " << req.Request->GetBody() << Endl;
        }
        for (size_t i = 0; i < fetchingRequests.size(); ++i) {
            NHttpFetcher::TResponse::TRef resp = fetchingRequests[i]->Wait();
            LOG(INFO) << "Push service response: " << resp->Data << Endl;
        }
    } else if (auto error = std::get_if<TError>(&requestsVariant)) {
        LOG(ERR) << "Error " << error->ToJson() << " with push of type " << event << Endl;
    } else {
        LOG(ERR) << "Strange error with push of type " << event << Endl;
    }
}

} // namespace NBASS::NPushNotification
