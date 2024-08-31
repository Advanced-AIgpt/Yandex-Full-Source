#include "subscriptions_manage_http_request.h"

#include <alice/matrix/notificator/library/subscriptions_info/subscriptions_info.h>
#include <alice/matrix/notificator/library/utils/utils.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_value.h>


namespace NMatrix::NNotificator {

namespace {

::NNotificator::TManageSubscription::EMethod GetSubscriptionMethodFromString(const TString& method) {
    // As is from python https://a.yandex-team.ru/svn/trunk/arcadia/alice/uniproxy/library/notificator/subscribes.py?rev=r9507130#L114
    // Do not compare with unsubscribe
    return method == "subscribe" ? ::NNotificator::TManageSubscription::ESubscribe : ::NNotificator::TManageSubscription::EUnsubscribe;
}

} // namespace

TSubscriptionsManageHttpRequest::TSubscriptionsManageHttpRequest(
    std::atomic<size_t>& requestCounterRef,
    TRtLogClient& rtLogClient,
    const NNeh::IRequestRef& request,
    TPushesAndNotificationsClient& pushesAndNotificationsClient
)
    : THttpRequest(
        NAME,
        requestCounterRef,
        /* needThreadSafeLogFrame = */ false,
        rtLogClient,
        request,
        [](NNeh::NHttp::ERequestType method) {
            return
                NNeh::NHttp::ERequestType::Get == method ||
                NNeh::NHttp::ERequestType::Post == method
            ;
        }
    )
    , PushesAndNotificationsClient_(pushesAndNotificationsClient)
{
    if (IsFinished()) {
        return;
    }

    NEvClass::TMatrixNotificatorSubscriptionsManageHttpRequestData event;
    TMaybe<TStringBuilder> parseError = Nothing();
    auto updateParseError = [&parseError](const TString& error) {
        if (!parseError.Defined()) {
            parseError.ConstructInPlace();
            *parseError << error;
        } else {
            *parseError << TString::Join("; ", error);
        }
    };

    if (NNeh::NHttp::ERequestType::Get == Method_) {
        const TCgiParameters cgi(HttpRequest_->Cgi());

        if (const auto puidIt = cgi.Find("puid"); puidIt != cgi.end()) {
            Request_.SetPuid(puidIt->second);
        } else {
            updateParseError("'puid' param not found");
        }

        if (const auto subscriptionIdIt = cgi.Find("subscription_id"); subscriptionIdIt != cgi.end()) {
            if (auto parseResult = TryParseFromString(subscriptionIdIt->second, "subscription id")) {
                Request_.SetSubscriptionId(parseResult.Success());
            } else {
                updateParseError(parseResult.Error());
            }
        } else {
            updateParseError("'subscription_id' param not found");
        }

        if (const auto methodIt = cgi.Find("method"); methodIt != cgi.end()) {
            Request_.SetMethod(GetSubscriptionMethodFromString(methodIt->second));
        } else {
            updateParseError("'method' param not found");
        }

        if (!parseError.Defined()) {
            event.SetCgiRequest(TString(HttpRequest_->Body()));
        }
    } else {
        const THttpInputHeader* contentType = HttpRequest_->Headers().FindHeader("Content-Type");

        // As is from python https://a.yandex-team.ru/svn/trunk/arcadia/alice/uniproxy/library/notificator/subscribes.py?rev=r9507130#L114
        // Do not use RequestContentIsJson_ here, this handler, unlike the others, compares content-type with 'application/protobuf'
        if (!contentType || contentType->Value() == "application/protobuf") {
            if (!Request_.ParseFromArray(HttpRequest_->Body().data(), HttpRequest_->Body().size())) {
                updateParseError("Unable to parse proto");
            }
        } else {
            try {
                NJson::TJsonValue jsonBody;
                NJson::ReadJsonTree(HttpRequest_->Body(), &jsonBody, true);

                if (const auto* puid = jsonBody.GetValueByPath("puid"); puid && puid->IsString()) {
                    Request_.SetPuid(puid->GetString());
                } else {
                    updateParseError("'puid' param not found");
                }

                if (const auto* subscriptionId = jsonBody.GetValueByPath("subscription_id"); subscriptionId && subscriptionId->IsUInteger()) {
                    Request_.SetSubscriptionId(subscriptionId->GetUInteger());
                } else {
                    updateParseError("'subscription_id' param not found");
                }

                if (const auto* method = jsonBody.GetValueByPath("method"); method && method->IsString()) {
                    Request_.SetMethod(GetSubscriptionMethodFromString(method->GetString()));
                } else {
                    updateParseError("'method' param not found");
                }

            } catch (...) {
                updateParseError(CurrentExceptionMessage());
            }

            if (!parseError.Defined()) {
                event.SetJsonRequest(TString(HttpRequest_->Body()));
            }
        }
    }

    if (parseError.Defined()) {
        event.SetUnparsedRawRequest(TString(HttpRequest_->Body()));
    }

    event.MutableProtoRequest()->CopyFrom(Request_);
    LogContext_.LogEventInfo<NEvClass::TMatrixNotificatorSubscriptionsManageHttpRequestData>(event);

    if (parseError) {
        SetError(*parseError, 400);
        IsFinished_ = true;
        return;
    }

    if (Request_.GetPuid().empty()) {
        SetError("Puid is empty", 400);
        IsFinished_ = true;
        return;
    }

    if (!GetSubscriptionsInfo().HasSubscription(Request_.GetSubscriptionId())) {
        SetError("Unknown subscription id", 400);
        IsFinished_ = true;
        return;
    }
}

NThreading::TFuture<void> TSubscriptionsManageHttpRequest::ServeAsync() {
    NThreading::TFuture<TExpected<void, TString>> resFut;
    if (Request_.GetMethod() == ::NNotificator::TManageSubscription::ESubscribe) {
        resFut = PushesAndNotificationsClient_.SubscribeUser(
            Request_.GetPuid(),
            Request_.GetSubscriptionId(),
            LogContext_,
            Metrics_
        );
    } else {
        resFut = PushesAndNotificationsClient_.UnsubscribeUser(
            Request_.GetPuid(),
            Request_.GetSubscriptionId(),
            LogContext_,
            Metrics_
        );
    }

    return resFut.Apply(
        [this](const NThreading::TFuture<TExpected<void, TString>>& fut) {
            const auto& res = fut.GetValueSync();
            if (!res) {
                SetError(res.Error(), 500);
            }
        }
    );
}

TSubscriptionsManageHttpRequest::TReply TSubscriptionsManageHttpRequest::GetReply() const {
    return TReply(GetReplyData(), THttpHeaders(), 200);
}

TString TSubscriptionsManageHttpRequest::GetReplyData() const {
    return R"({"code": 200})";
}

} // namespace NMatrix::NNotificator
