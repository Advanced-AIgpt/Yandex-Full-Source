#include "get_last_order_pa.h"
#include "auth.h"
#include "http_utils.h"
#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/scenarios/food/backend/proto/requests.pb.h>

namespace NAlice::NHollywood::NFood {

namespace NApiGetLastOrderPA {

    static const TString REQUEST_PARAMS = "get_last_order_pa_request_params";
    static const TString RTLOG_TOKEN = "get_last_order_pa_rtlog_token";
    static const TString HTTP_RESPONSE = "get_last_order_pa_http_response";

    void AddRequest(TScenarioHandleContext& ctx, const TAuthInput& authInput) {
        // More than one request will cause an error in apphost. Single request is enough
        if (ctx.ServiceCtx.HasProtobufItem(REQUEST_PARAMS)) {
            return;
        }
        LOG_INFO(ctx.Ctx.Logger()) << "NApiGetLastOrderPA::AddRequest authInput: " << DbgDumpDeep(authInput);
        TGetLastOrderPARequestParams params;
        params.SetTaxiUid(authInput.TaxiUid);
        if (authInput.TaxiUid.empty()) {
            NApiGetTaxiUid::AddRequest(ctx, authInput.Phone, authInput.YandexUid);
        }
        LOG_INFO(ctx.Ctx.Logger()) << "Add request " << REQUEST_PARAMS << ": " << JsonFromProto(params);
        ctx.ServiceCtx.AddProtobufItem(params, REQUEST_PARAMS);
    }

    TString TRequestHandle::Name() const {
        return "get_last_order_pa_prepare";
    }

    void TRequestHandle::Do(TScenarioHandleContext& ctx) const {
        const auto params = GetOnlyProtoOrThrow<TGetLastOrderPARequestParams>(ctx.ServiceCtx, REQUEST_PARAMS);
        const TString taxiUid = params.GetTaxiUid().empty() ? NApiGetTaxiUid::GetTaxiUid(ctx) : params.GetTaxiUid();
        if (taxiUid.empty()) {
            LOG_ERROR(ctx.Ctx.Logger()) << "No TaxiUid.";
            return;
        }
        const THttpProxyRequest httpRequest = PrepareHttpRequest(ctx, "/orders", MakeHeadersWithTaxiUid(taxiUid), Nothing(), /* useOAuth= */ true);
        LOG_DEBUG(ctx.Ctx.Logger()) << SerializeProtoText(httpRequest.Request, /* singleLineMode= */ false);
        AddHttpRequestItems(ctx, httpRequest, "http_request", RTLOG_TOKEN);
    }

    TResponseData ReadResponse(TScenarioHandleContext& ctx) {
        const auto params = GetOnlyProtoOrThrow<TGetLastOrderPARequestParams>(ctx.ServiceCtx, REQUEST_PARAMS);
        const TMaybe<TJsonHttpResponse> response = RetireHttpResponseJsonExtendedMaybe(ctx, HTTP_RESPONSE, RTLOG_TOKEN);
        const TString taxiUid = params.GetTaxiUid().empty() ? NApiGetTaxiUid::GetTaxiUid(ctx) : params.GetTaxiUid();

        if (taxiUid.empty()) {
             return {.Error = EError::AUTHORIZATION_FAILED};
        }
        if (!response.Defined()) {
            return {.Error = EError::NO_RESPONSE, .LastOrder = {}, .TaxiUid = taxiUid};
        }

        const NJson::TJsonValue* lastOrder = nullptr;
        for (const NJson::TJsonValue& order : response->Body.GetArray()) {
            if (lastOrder == nullptr || (*lastOrder)["created_at"].GetString() < order["created_at"].GetString()) {
                lastOrder = &order;
            }
        }
        if (lastOrder == nullptr) {
            return {.Error = EError::NO_ORDER, .LastOrder = {}, .TaxiUid = taxiUid};
        }
        return {.Error = EError::SUCCESS, .LastOrder = *lastOrder, .TaxiUid = taxiUid};
    }

} // namespace NApiGetLastOrderPA

} // namespace NAlice::NHollywood::NFood
