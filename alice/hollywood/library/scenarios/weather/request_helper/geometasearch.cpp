#include "geometasearch.h"

#include <alice/hollywood/library/resources/geobase.h>

#include <alice/library/geo_resolver/geo_position.h>
#include <alice/library/geo_resolver/geo_response_parser.h>

#include <apphost/lib/proto_answers/http.pb.h>

namespace NAlice::NHollywood::NWeather {

namespace {

constexpr TStringBuf GEOMETASEARCH_REQUEST_ITEM = "geometasearch_http_request";
constexpr TStringBuf GEOMETASEARCH_RESPONSE_ITEM = "geometasearch_http_response";

NAppHostHttp::THttpRequest ConstructGeometasearchHttpRequest(const TStringBuf lang,
                                                             const TScenarioRunRequestWrapper requestWrapper,
                                                             TStringBuf searchText)
{
    NAppHostHttp::THttpRequest request;
    request.SetScheme(NAppHostHttp::THttpRequest::Http);
    request.SetMethod(NAppHostHttp::THttpRequest::Get);

    // Poor Man's "NormalizedText"
    if (searchText.SkipPrefix("в ") || searchText.SkipPrefix("на ")) {
        searchText.ChopSuffix("е");
    }

    TCgiParameters cgi;
    cgi.InsertEscaped("gta", "geoid");
    cgi.InsertEscaped("gta", "accuracy");
    cgi.InsertEscaped("gta", "ll");
    cgi.InsertEscaped("ms", "pb");
    cgi.InsertEscaped("type", "geo,biz");
    cgi.InsertEscaped("lang", lang);
    // OLD TODO: we should use "geowhere", but right now it doesn't work properly
    // (see https://st.yandex-team.ru/GEOSEARCH-3917)
    cgi.InsertEscaped("text", searchText);
    cgi.InsertEscaped("results", "20");
    cgi.InsertUnescaped("p", "0");
    if (const auto position = InitGeoPositionFromRequest(requestWrapper.BaseRequestProto())) {
        // Specify user`s location, otherwise we may receive result in another city
        const TString& llStr = position->GetLonLatString();
        cgi.InsertEscaped("ll", llStr);
        cgi.InsertEscaped("ull", llStr);
    }

    cgi.InsertEscaped("origin", "assistant_bass"); // TODO(sparkle): redefine origin?

    request.SetPath(TString::Join("/yandsearch?", cgi.Print()));
    return request;
}

} // namespace

TBeforeGeometasearchRequestHelper::TBeforeGeometasearchRequestHelper(TScenarioHandleContext& ctx)
    : Ctx_{ctx}
{}

void TBeforeGeometasearchRequestHelper::AddRequest(const TStringBuf text) {
    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioRunRequest>(Ctx_.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper requestWrapper{requestProto, Ctx_.ServiceCtx};

    auto request = ConstructGeometasearchHttpRequest(Ctx_.RequestMeta.GetLang(), requestWrapper, text);
    Ctx_.ServiceCtx.AddProtobufItem(std::move(request), GEOMETASEARCH_REQUEST_ITEM);
}

TAfterGeometasearchRequestHelper::TAfterGeometasearchRequestHelper(TScenarioHandleContext& ctx)
    : Ctx_{ctx}
{}

TWeatherErrorOr<NGeobase::TId> TAfterGeometasearchRequestHelper::TryParseGeoId(const TStringBuf text) const {
    if (!Ctx_.ServiceCtx.HasProtobufItem(GEOMETASEARCH_RESPONSE_ITEM)) {
        return TWeatherError{EWeatherErrorCode::SYSTEM} << "No GeoMetasearch answer where expected";
    }

    const auto& response = Ctx_.ServiceCtx.GetOnlyProtobufItem<NAppHostHttp::THttpResponse>(GEOMETASEARCH_RESPONSE_ITEM);
    if (response.GetStatusCode() != 200) {
        return TWeatherError{EWeatherErrorCode::SYSTEM} << "No GeoMetasearch HTTP 200 OK";
    }

    ::yandex::maps::proto::common2::response::Response pbResponse;
    if (!pbResponse.ParseFromString(response.GetContent())) {
        return TWeatherError{EWeatherErrorCode::SYSTEM} << "Can't parse GeoMetasearch response";
    }

    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioRunRequest>(Ctx_.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper requestWrapper{requestProto, Ctx_.ServiceCtx};

    const auto& interfaces = requestWrapper.BaseRequestProto().GetInterfaces();
    const auto& geobase = Ctx_.Ctx.GlobalContext().CommonResources().Resource<TGeobaseResource>().GeobaseLookup();
    TVector<NSc::TValue> objectInfos;
    NAlice::ParseGeoObjectPage(pbResponse, geobase, requestWrapper.ClientInfo(),
        GetUserLocation(requestWrapper), EContentSettings::without, text, /* resultIndexOnPage = */ 1,
        /* docsCount = */ 1, objectInfos, /* privateNavigatorKey = */ TStringBuf(),
        interfaces.GetCanOpenLinkSearchViewport(), interfaces.GetCanOpenLinkIntent());

    if (objectInfos.empty()) {
        return TWeatherError{EWeatherErrorCode::NOGEOFOUND} << "Empty ObjectInfos after parsing GeoMetasearch response in Weather";
    }

    const NSc::TValue& location = objectInfos.front();
    NGeobase::TId geoId = location["geoid"].GetIntNumber(NGeobase::UNKNOWN_REGION);
    if (!NAlice::IsValidId(geoId)) {
        return TWeatherError{EWeatherErrorCode::NOGEOFOUND} << "No geo found: " << text;
    }

    LOG_INFO(Ctx_.Ctx.Logger()) << "Successfully got GeoId " << geoId << " from GeoMetasearch";
    return geoId;
}

} // NAlice::NHollywood::NWeather
