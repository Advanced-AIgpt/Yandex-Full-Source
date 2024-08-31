#include "get_address.h"

#include "auth.h"
#include "http_utils.h"

#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/scenarios/food/backend/proto/requests.pb.h>

#include <alice/megamind/protos/common/location.pb.h>

#include <alice/library/json/json.h>

#include <library/cpp/geo/geo.h>
#include <library/cpp/geo/util.h>

#include <cmath>

using namespace std;

namespace NAlice::NHollywood::NFood {

namespace {

constexpr int DEFAULT_LOCATION_EPS_M = 250;

double GetDistanceInMeters(double lat1, double lon1, double lat2, double lon2) {
    const auto distanceInDeg = NGeo::TGeoPoint(lat1, lon1).Distance(NGeo::TGeoPoint(lat2, lon2));
    return NGeo::GetMetersFromDeg(distanceInDeg);
}

} // namespace

namespace NApiGetAddress {

static const TString REQUEST_PARAMS = "get_eater_address_request_params";
static const TString RTLOG_TOKEN = "get_eater_address_rtlog_token";
static const TString HTTP_RESPONSE = "get_eater_address_http_response";

static const TString GET_ADDRESS_PATH = "/user/addresses";

void AddRequest(TScenarioHandleContext& ctx, const TLocation& location, const TAuthInput& authInput) {
    // More than one request will cause an error in apphost. Single request is enough
    if (ctx.ServiceCtx.HasProtobufItem(REQUEST_PARAMS)) {
        return;
    }
    LOG_DEBUG(ctx.Ctx.Logger()) << "NApiGetAddress::AddRequest authInput: " << DbgDumpDeep(authInput);
    TGetAddressRequestParams params;
    params.SetYandexUid(authInput.YandexUid);
    params.SetTaxiUid(authInput.TaxiUid);
    params.SetLat(location.GetLat());
    params.SetLon(location.GetLon());
    if (authInput.TaxiUid.empty()) {
        NApiGetTaxiUid::AddRequest(ctx, authInput.Phone, authInput.YandexUid);
    }
    LOG_INFO(ctx.Ctx.Logger()) << "Add request " << REQUEST_PARAMS << ": " << JsonFromProto(params);
    ctx.ServiceCtx.AddProtobufItem(params, REQUEST_PARAMS);
}

TString TRequestHandle::Name() const {
    return "get_eater_address_prepare";
}

void TRequestHandle::Do(TScenarioHandleContext& ctx) const {
    const auto params = GetOnlyProtoOrThrow<TGetAddressRequestParams>(ctx.ServiceCtx, REQUEST_PARAMS);
    const TString taxiUid = params.GetTaxiUid().empty() ? NApiGetTaxiUid::GetTaxiUid(ctx) : params.GetTaxiUid();
    if (taxiUid.empty()) {
        LOG_ERROR(ctx.Ctx.Logger()) << "No TaxiUid.";
        return;
    }
    const THttpProxyRequest httpRequest =                   //
        PrepareHttpRequest(ctx,                             //
                           GET_ADDRESS_PATH,                //
                           MakeHeadersWithTaxiUid(taxiUid), //
                           Nothing(),                       //
                           /* useOAuth= */ true);
    LOG_DEBUG(ctx.Ctx.Logger()) << SerializeProtoText(httpRequest.Request, /* singleLineMode= */ false);
    AddHttpRequestItems(ctx, httpRequest, "http_request", RTLOG_TOKEN);
}

TResponseData ReadResponse(TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& req) {
    const auto params = GetOnlyProtoOrThrow<TGetAddressRequestParams>(ctx.ServiceCtx, REQUEST_PARAMS);
    const auto response = RetireHttpResponseJsonExtendedMaybe(ctx, HTTP_RESPONSE, RTLOG_TOKEN, /* logBody = */ true);
    const TString taxiUid = params.GetTaxiUid().empty() ? NApiGetTaxiUid::GetTaxiUid(ctx) : params.GetTaxiUid();

    if (taxiUid.empty()) {
        return {.Error = EError::AUTHORIZATION_FAILED};
    }
    if (!response.Defined()) {
        return {.Error = EError::NO_RESPONSE};
    }

    for (const NJson::TJsonValue& address : response->Body.GetArray()) {
        const double lat = address["location"]["latitude"].GetDouble();
        const double lon = address["location"]["longitude"].GetDouble();

        int locationEpsMeters = DEFAULT_LOCATION_EPS_M;
        if (const auto value = req.GetValueFromExpPrefix(EXP_HW_FOOD_USER_ADDRESS_ERROR_MARGIN_PREFIX);
            value.Defined()) {
            TryFromString(*value, locationEpsMeters);
        }
        if (GetDistanceInMeters(lat, lon, params.GetLat(), params.GetLon()) < locationEpsMeters) {
            return {
                .Error = EError::SUCCESS,
                .Address = address,
            };
        }
    }

    return {.Error = EError::ADDRESS_NOT_FOUND};
}

} // namespace NApiGetAddress

} // namespace NAlice::NHollywood::NFood
