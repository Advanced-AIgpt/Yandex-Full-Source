#include "auth.h"

#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/scenarios/food/backend/http_utils.h>
#include <alice/hollywood/library/scenarios/food/backend/proto/requests.pb.h>

namespace NAlice::NHollywood::NFood {

namespace NApiGetTaxiUid {

    static const TString REQUEST_PARAMS = "get_taxi_uid_request_params";
    static const TString RTLOG_TOKEN = "get_taxi_uid_rtlog_token";
    static const TString HTTP_RESPONSE = "get_taxi_uid_http_response";

    void AddRequest(TScenarioHandleContext& ctx, const TString& phone, const TString& yandexUid) {
        // More than one request will cause an error in apphost. Single request is enough
        if (ctx.ServiceCtx.HasProtobufItem(REQUEST_PARAMS)) {
            return;
        }
        TGetTaxiUidRequestParams params;
        params.SetPhone(phone);
        params.SetYandexUid(yandexUid);
        LOG_INFO(ctx.Ctx.Logger()) << "Add request " << REQUEST_PARAMS << ": " << JsonFromProto(params);
        ctx.ServiceCtx.AddProtobufItem(params, REQUEST_PARAMS);
    }

    TString TRequestHandle::Name() const {
        return "get_taxi_uid_prepare";
    }

    void TRequestHandle::Do(TScenarioHandleContext& ctx) const {
        const TGetTaxiUidRequestParams params = GetOnlyProtoOrThrow<TGetTaxiUidRequestParams>(ctx.ServiceCtx, REQUEST_PARAMS);
        NJson::TJsonValue body;
        const THttpProxyRequest httpRequest = PrepareHttpRequest(ctx, "", {}, body, /* useOAuth= */ true);
        AddHttpRequestItems(ctx, httpRequest, "http_request", RTLOG_TOKEN);
    }

    TResponseData ReadResponse(TScenarioHandleContext& ctx) {
        const TMaybe<TJsonHttpResponse> response = RetireHttpResponseJsonExtendedMaybe(ctx, HTTP_RESPONSE, RTLOG_TOKEN, /* logBody= */ true);
        if (!response.Defined()) {
            LOG_ERROR(ctx.Ctx.Logger()) << "Failed to get taxi uid.";
            return {.Error = EError::NO_RESPONSE};
        }
        const TString taxiUid = NFood::GetTaxiUid(response->Headers);
        if (taxiUid.empty()) {
            return {.Error = EError::FAILED_TO_AUTHENTICATE};
        }
        return {.Error = EError::SUCCESS, .TaxiUid = taxiUid};
    }

    TString GetTaxiUid(TScenarioHandleContext& ctx) {
        NApiGetTaxiUid::TResponseData response = NApiGetTaxiUid::ReadResponse(ctx);
        if (response.Error != NApiGetTaxiUid::EError::SUCCESS) {
            LOG_ERROR(ctx.Ctx.Logger()) << "Failed to autheticate user with passenger authorizer.";
            return {};
        }
        return response.TaxiUid;
    }

} // namespace NApiGetTaxiUid

} // namespace NAlice::NHollywood::NFood
