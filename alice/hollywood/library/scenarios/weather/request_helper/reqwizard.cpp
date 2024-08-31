#include "reqwizard.h"

#include <apphost/lib/proto_answers/http.pb.h>

namespace NAlice::NHollywood::NWeather {

namespace {

constexpr TStringBuf REQWIZARD_REQUEST_ITEM = "reqwizard_http_request";
constexpr TStringBuf REQWIZARD_RESPONSE_ITEM = "reqwizard_http_response";

constexpr TStringBuf WIZCLIENT = "assistant.yandex";
constexpr TStringBuf FORMAT = "json";

struct TGeoAddrTokens;

using TGeoTokens = TMap<TString, TGeoAddrTokens>;
using TTokensWithWeights = TMap<NGeobase::TId, size_t>;

bool BestIdComparator(const std::pair<NGeobase::TId, size_t>& l, const std::pair<NGeobase::TId, size_t>& r) {
    return l.second < r.second;
}

NGeobase::TId GetTheBestIds(const TTokensWithWeights& ids) {
    auto it = std::max_element(
        ids.cbegin(),
        ids.cend(),
        BestIdComparator
    );
    return it == ids.cend() ? NGeobase::UNKNOWN_REGION : it->first;
}

struct TGeoAddrTokens {
    TGeoAddrTokens(bool sameAsQuery, TStringBuf normalized)
        : SameAsQuery(sameAsQuery)
        , Normalized(normalized)
    {
    }

    NGeobase::TId BestId() const {
        if (BestIds.size()) {
            NGeobase::TId geoId = GetTheBestIds(BestIds);
            if (NAlice::IsValidId(geoId)) {
                return geoId;
            }
        }
        return NGeobase::UNKNOWN_REGION;
    }

    const bool SameAsQuery;
    const TString Normalized;
    TTokensWithWeights BestIds;
    TTokensWithWeights InheritedIds;
};

bool GetOneBestGeoId(const TGeoTokens& tokens, NGeobase::TId& id) {
    if (tokens.size() != 1) {
        return false;
    }
    id = tokens.cbegin()->second.BestId();
    return true;
}

NAppHostHttp::THttpRequest ConstructReqwizardHttpRequest(const TStringBuf lang,
                                                         const TUserLocation& userLocation,
                                                         const TStringBuf text)
{
    NAppHostHttp::THttpRequest request;
    request.SetScheme(NAppHostHttp::THttpRequest::Http);
    request.SetMethod(NAppHostHttp::THttpRequest::Get);

    TCgiParameters cgi;
    cgi.InsertEscaped("format", FORMAT);
    cgi.InsertEscaped("wizclient", WIZCLIENT);
    cgi.InsertEscaped("text", text);
    cgi.InsertUnescaped("tld", userLocation.UserTld());
    cgi.InsertUnescaped("lr", ToString(userLocation.UserRegion()));

    if (!lang.Empty()) {
        cgi.InsertUnescaped("uil", lang);
    }

    request.SetPath(TString::Join("/wizard?", cgi.Print()));
    return request;
}

} // namespace

TBeforeReqwizardRequestHelper::TBeforeReqwizardRequestHelper(TScenarioHandleContext& ctx)
    : Ctx_{ctx}
{}

void TBeforeReqwizardRequestHelper::AddRequest(const TStringBuf text) {
    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioRunRequest>(Ctx_.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper requestWrapper{requestProto, Ctx_.ServiceCtx};

    auto request = ConstructReqwizardHttpRequest(Ctx_.RequestMeta.GetLang(), GetUserLocation(requestWrapper), text);
    Ctx_.ServiceCtx.AddProtobufItem(std::move(request), REQWIZARD_REQUEST_ITEM);
}

TAfterReqwizardRequestHelper::TAfterReqwizardRequestHelper(TScenarioHandleContext& ctx)
    : Ctx_{ctx}
{}

TWeatherErrorOr<NGeobase::TId> TAfterReqwizardRequestHelper::TryParseGeoId() const {
    auto& logger = Ctx_.Ctx.Logger();

    if (!Ctx_.ServiceCtx.HasProtobufItem(REQWIZARD_RESPONSE_ITEM)) {
        return TWeatherError{EWeatherErrorCode::SYSTEM} << "No ReqWizard answer where expected";
    }

    const auto& response = Ctx_.ServiceCtx.GetOnlyProtobufItem<NAppHostHttp::THttpResponse>(REQWIZARD_RESPONSE_ITEM);
    if (response.GetStatusCode() != 200) {
        return TWeatherError{EWeatherErrorCode::SYSTEM} << "No ReqWizard HTTP 200 OK";
    }

    auto rwd = NSc::TValue::FromJson(response.GetContent());
    LOG_INFO(logger) << "ReqWizard answer: " << rwd.ToJson();

    const auto& gar = rwd["rules"]["GeoAddr"];
    if (gar.IsNull()) {
        return TWeatherError{EWeatherErrorCode::NOGEOFOUND} << "No geo address found at ReqWizard response";
    }

    const auto& ua = gar["UnfilteredAnswer"];
    if (ua.IsNull()) {
        return TWeatherError{EWeatherErrorCode::NOGEOFOUND} << "No unfiltered answer found in geoaddr rule";
    }

    TVector<NSc::TValue> found;
    if (ua.IsArray()) {
        for (const NSc::TValue& ans : ua.GetArray()) {
            found.emplace_back(NSc::TValue::FromJson(ans.GetString()));
        }
    } else {
        found.emplace_back(NSc::TValue::FromJson(ua.GetString()));
    }

    const i64 queryNumTokens = rwd["markup"]["Tokens"].ArraySize();

    TGeoTokens geoTokens;
    for (const NSc::TValue& garItem : found) {
        const NSc::TValue& body = garItem["Body"];
        if (body["Weight"].GetNumber() < 0.1) {
            continue;
        }
        NGeobase::TId bgid = body["BestGeo"].GetIntNumber(NGeobase::UNKNOWN_REGION);
        const i64 pos = garItem["Pos"].ForceIntNumber();
        const i64 len = garItem["Length"].ForceIntNumber();

        if (IsValidId(bgid)) {
            const TString key{TStringBuilder() << pos << len}; // FIXME key
            TGeoAddrTokens* geo = geoTokens.FindPtr(key);
            if (!geo) {
                geo = &geoTokens.emplace(
                    std::piecewise_construct,
                    std::forward_as_tuple(key),
                    std::forward_as_tuple(len == queryNumTokens, garItem["NormalizedText"].GetString())
                ).first->second;
            }
            ++geo->BestIds[bgid];
        }
    }

    NGeobase::TId geoId;
    if (!GetOneBestGeoId(geoTokens, geoId)) {
        return TWeatherError{EWeatherErrorCode::NOGEOFOUND} << "Didn't find geoId at ReqWizard response";
    }

    LOG_INFO(logger) << "Successfully got GeoId " << geoId << " from ReqWizard";
    return geoId;
}

} // NAlice::NHollywood::NWeather
