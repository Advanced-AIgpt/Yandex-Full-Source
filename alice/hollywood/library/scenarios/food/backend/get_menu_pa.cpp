#include "get_menu_pa.h"
#include "auth.h"
#include "http_utils.h"
#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/scenarios/food/backend/proto/requests.pb.h>
#include <alice/megamind/protos/common/location.pb.h>
#include <library/cpp/string_utils/quote/quote.h>
#include <util/charset/wide.h>
#include <util/string/join.h>

namespace NAlice::NHollywood::NFood {

namespace {

void AddHttpProxyRequestIfPossible(TScenarioHandleContext& ctx, const TString& path, const TString& taxiUid, const TString& rtlogToken) {
    if (taxiUid.empty()) {
        LOG_ERROR(ctx.Ctx.Logger()) << "No TaxiUid.";
        return;
    }

    const THttpProxyRequest httpRequest = PrepareHttpRequest(ctx, path, MakeHeadersWithTaxiUid(taxiUid), Nothing(), /* useOAuth= */ true);
    AddHttpRequestItems(ctx, httpRequest, "http_request", rtlogToken);
}


} // namespace

namespace NApiFindPlacePA {

    static const TString REQUEST_PARAMS = "find_place_pa_request_params";
    static const TString RTLOG_TOKEN = "find_place_pa_rtlog_token";
    static const TString HTTP_RESPONSE = "find_place_pa_http_response";

    void AddRequest(TScenarioHandleContext& ctx, const TLocation& location, const TAuthInput& authInput) {
        // More than one request will cause an error in apphost. Single request is enough
        if (ctx.ServiceCtx.HasProtobufItem(REQUEST_PARAMS)) {
            return;
        }
        LOG_INFO(ctx.Ctx.Logger()) << "NApiFindPlacePA::AddRequest authInput: " << DbgDumpDeep(authInput);
        TFindPlacePARequestParams params;
        params.SetLat(location.GetLat());
        params.SetLon(location.GetLon());
        params.SetPlaceName("макдоналдс"); // TODO(the0): set name from input semantic frame slot after normalization
        // params.SetPlaceName("white star burger"); // TODO(the0): set name from input semantic frame slot after normalization
        params.SetTaxiUid(authInput.TaxiUid);
        if (authInput.TaxiUid.empty()) {
            NApiGetTaxiUid::AddRequest(ctx, authInput.Phone, authInput.YandexUid);
        }
        LOG_INFO(ctx.Ctx.Logger()) << "Add request " << REQUEST_PARAMS << ": " << JsonFromProto(params);
        ctx.ServiceCtx.AddProtobufItem(params, REQUEST_PARAMS);
    }

    TString TRequestHandle::Name() const {
        return "find_place_pa_prepare";
    }

    void TRequestHandle::Do(TScenarioHandleContext& ctx) const {
        const TFindPlacePARequestParams params = GetOnlyProtoOrThrow<TFindPlacePARequestParams>(ctx.ServiceCtx, REQUEST_PARAMS);
        const TString taxiUid = params.GetTaxiUid().empty() ? NApiGetTaxiUid::GetTaxiUid(ctx) : params.GetTaxiUid();

        const TString path = TStringBuilder{}
            << "/catalog"
            << "?" << "longitude=" << params.GetLon()
            << "&" << "latitude=" << params.GetLat();
            // << "&" << "search=" << CGIEscapeRet(params.GetPlaceName());

        LOG_INFO(ctx.Ctx.Logger()) << "Start do " << REQUEST_PARAMS;
        AddHttpProxyRequestIfPossible(ctx, path, taxiUid, RTLOG_TOKEN);
    }

    static bool Matches(const TString& request, const TString& name) {
        return ToLowerRet(UTF8ToWide(name)).Contains(ToLowerRet(UTF8ToWide(request)));
    }

    TResponseData ReadResponse(TScenarioHandleContext& ctx) {
        const TMaybe<TJsonHttpResponse> placesNearbyResponse = RetireHttpResponseJsonExtendedMaybe(ctx, HTTP_RESPONSE, RTLOG_TOKEN, false);
        const TFindPlacePARequestParams params = GetOnlyProtoOrThrow<TFindPlacePARequestParams>(ctx.ServiceCtx, REQUEST_PARAMS);
        const TString taxiUid = params.GetTaxiUid().empty() ? NApiGetTaxiUid::GetTaxiUid(ctx) : params.GetTaxiUid();

        if (taxiUid.empty()) {
             return {.Error = EError::AUTHORIZATION_FAILED};
        }

        if (!placesNearbyResponse.Defined()) {
            LOG_ERROR(ctx.Ctx.Logger()) << "Failed to get places nearby.";
            return {.Error = EError::NO_RESPONSE, .PlaceSlug = {}, .TaxiUid = taxiUid};
        }

        const TFindPlacePARequestParams request = GetLastProtoOrThrow<TFindPlacePARequestParams>(ctx.ServiceCtx, REQUEST_PARAMS);
        LOG_INFO(ctx.Ctx.Logger()) << "Place name request: " << request.GetPlaceName();

        TVector<TString> placeNames;
        for (const NJson::TJsonValue& place : placesNearbyResponse->Body["payload"]["foundPlaces"].GetArray()) {
            const TString placeName = place["place"]["name"].GetString();
            placeNames.push_back(placeName);
            if (Matches(request.GetPlaceName(), placeName)) {
                const TString slug = place["place"]["slug"].GetString();
                LOG_INFO(ctx.Ctx.Logger()) << "Place is found: " << slug;
                return {.Error = EError::SUCCESS, .PlaceSlug = slug, .TaxiUid = taxiUid};
            }
        }

        LOG_ERROR(ctx.Ctx.Logger()) << "Requested place not found. Found places: " << JoinSeq("; ", placeNames);
        return {.Error = EError::PLACE_NOT_FOUND, .PlaceSlug = {}, .TaxiUid = taxiUid};
    }

} // namespace NApiFindPlacePA

namespace NApiGetMenuPA {

    static const TString REQUEST_PARAMS = "get_menu_pa_request_params";
    static const TString RTLOG_TOKEN = "get_menu_pa_rtlog_token";
    static const TString HTTP_RESPONSE = "get_menu_pa_http_response";

    void AddRequest(TScenarioHandleContext& ctx, const TLocation& location, const TAuthInput& authInput, const TString& placeSlug) {
        // More than one request will cause an error in apphost. Single request is enough
        if (ctx.ServiceCtx.HasProtobufItem(REQUEST_PARAMS)) {
            return;
        }
        LOG_INFO(ctx.Ctx.Logger()) << "NApiGetMenuPA::AddRequest authInput: " << DbgDumpDeep(authInput) << " " << placeSlug;
        if (placeSlug.empty()) {
            NApiFindPlacePA::AddRequest(ctx, location, authInput);
        } else if (authInput.TaxiUid.empty()) {
            NApiGetTaxiUid::AddRequest(ctx, authInput.Phone, authInput.YandexUid);
        }
        TGetMenuPARequestParams params;
        params.SetTaxiUid(authInput.TaxiUid);
        params.SetPlaceSlug(placeSlug);
        LOG_INFO(ctx.Ctx.Logger()) << "Add request " << REQUEST_PARAMS << ": " << JsonFromProto(params);
        ctx.ServiceCtx.AddProtobufItem(params, REQUEST_PARAMS);
    }

    TString TRequestHandle::Name() const {
        return "get_menu_pa_prepare";
    }

    void TRequestHandle::Do(TScenarioHandleContext& ctx) const {
        const TGetMenuPARequestParams params = GetOnlyProtoOrThrow<TGetMenuPARequestParams>(ctx.ServiceCtx, REQUEST_PARAMS);
        const TString placeSlug = params.GetPlaceSlug().empty() ? NApiFindPlacePA::ReadResponse(ctx).PlaceSlug : params.GetPlaceSlug();
        const TString taxiUid = params.GetTaxiUid().empty() ? NApiGetTaxiUid::GetTaxiUid(ctx) : params.GetTaxiUid();
        const TString path = TStringBuilder{} << "/catalog/" << placeSlug << "/menu";

        LOG_INFO(ctx.Ctx.Logger()) << "Start do " << REQUEST_PARAMS << ": " << placeSlug << " " << taxiUid;
        AddHttpProxyRequestIfPossible(ctx, path, taxiUid, RTLOG_TOKEN);
    }

    TResponseData ReadResponse(TScenarioHandleContext& ctx) {
        TMaybe<TJsonHttpResponse> menuResponse = RetireHttpResponseJsonExtendedMaybe(ctx, HTTP_RESPONSE, RTLOG_TOKEN, false);
        const TGetMenuPARequestParams params = GetOnlyProtoOrThrow<TGetMenuPARequestParams>(ctx.ServiceCtx, REQUEST_PARAMS);
        const TString taxiUid = params.GetTaxiUid().empty() ? NApiGetTaxiUid::GetTaxiUid(ctx) : params.GetTaxiUid();

        if (taxiUid.empty()) {
             return {.Error = EError::AUTHORIZATION_FAILED};
        }

        if (!menuResponse.Defined()) {
            const NApiFindPlacePA::TResponseData place = NApiFindPlacePA::ReadResponse(ctx);
            if (place.Error == NApiFindPlacePA::EError::PLACE_NOT_FOUND) {
                return {.Error = EError::PLACE_NOT_FOUND, .Menu = {}, .TaxiUid = taxiUid};
            }
            return {.Error = EError::NO_RESPONSE, .Menu = {}, .TaxiUid = taxiUid};
        }
        return {
            .Error = EError::SUCCESS,
            .Menu = std::move(menuResponse->Body),
            .TaxiUid = taxiUid,
            .PlaceSlug = params.GetPlaceSlug().empty() ? NApiFindPlacePA::ReadResponse(ctx).PlaceSlug : params.GetPlaceSlug()
        };
    }

} // namespace NApiGetMenuPA

} // namespace NAlice::NHollywood::NFood
