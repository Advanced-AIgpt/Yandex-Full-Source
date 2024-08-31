#include "geo_resolver.h"

#include <alice/bass/libs/config/config.h>
#include <alice/bass/forms/geocoder.h>
#include <alice/bass/forms/geodb.h>
#include <alice/bass/forms/search/serp.h>
#include <alice/bass/forms/urls_builder.h>
#include <alice/bass/libs/globalctx/globalctx.h>
#include <alice/bass/libs/logging_v2/logger.h>

#include <library/cpp/timezone_conversion/convert.h>

#include <search/session/compression/report.h>

#include <util/datetime/base.h>
#include <util/generic/strbuf.h>
#include <library/cpp/cgiparam/cgiparam.h>

namespace NBASS {

namespace {

constexpr int PAGE_SIZE = 20;

void AddSpanForNavi(const TContext& ctx, TCgiParameters& cgi) {
    if (ctx.MetaClientInfo().IsNavigator()) {
        const auto& naviState = ctx.Meta().DeviceState().NavigatorState();
        if (naviState.HasSearchOptions() && naviState.SearchOptions().HasSpan()) {
            const auto& sw = naviState.SearchOptions().Span().SouthWest();
            const auto& ne = naviState.SearchOptions().Span().NorthEast();

            double width = abs(ne.Lon() - sw.Lon());
            double height = abs(ne.Lat() - sw.Lat());

            if (width > 0 && height > 0) {
                cgi.InsertUnescaped(TStringBuf("spn"), TStringBuilder() << Sprintf("%.5f", width) << "," << Sprintf("%.5f", height));
            }
        }
    }
}

} // anon namespace


TGeoObjectResolver::TGeoObjectResolver(TContext& ctx, const TStringBuf searchText, const TMaybe<TGeoPosition>& searchPos,
        const TStringBuf geoType, const TStringBuf sortBy, const TVector<TStringBuf>& businessFilters, const size_t resultIndex,
        const bool includeExperimentalMeta)
    : Ctx(ctx)
    , SearchText(searchText)
{
    const size_t pageNum = (resultIndex - 1) / PAGE_SIZE;
    ResultIndexOnPage = resultIndex - pageNum * PAGE_SIZE;
    static const TString pageSizeStr = ToString(PAGE_SIZE);

    TCgiParameters cgi;
    cgi.InsertEscaped(TStringBuf("gta"), TStringBuf("geoid"));
    cgi.InsertEscaped(TStringBuf("gta"), TStringBuf("accuracy"));
    cgi.InsertEscaped(TStringBuf("gta"), TStringBuf("ll"));
    cgi.InsertEscaped(TStringBuf("ms"), TStringBuf("pb"));
    cgi.InsertEscaped(TStringBuf("type"), geoType);
    cgi.InsertEscaped(TStringBuf("lang"), ctx.Meta().Lang());
    // TODO: we should use "geowhere", but right now it doesn't work properly
    // (see https://st.yandex-team.ru/GEOSEARCH-3917)
    cgi.InsertEscaped(TStringBuf("text"), searchText);
    cgi.InsertEscaped(TStringBuf("results"), pageSizeStr);
    cgi.InsertUnescaped(TStringBuf("p"), ToString(pageNum));
    if (searchPos) {
        // Specify user`s location, otherwise we may receive result in another city
        const TString& llStr = searchPos->GetLonLatString();
        cgi.InsertEscaped(TStringBuf("ll"), llStr);
        cgi.InsertEscaped(TStringBuf("ull"), llStr);
    }

    AddSpanForNavi(ctx, cgi);

    if (!sortBy.empty()) {
        cgi.InsertEscaped(TStringBuf("sort"), sortBy);
    }
    cgi.InsertEscaped(TStringBuf("origin"), GEOSEARCH_ORIGIN);

    if (includeExperimentalMeta) {
        cgi.InsertEscaped(TStringBuf("gta"), TStringBuf("matchedobjects/1.x"));
        cgi.InsertEscaped(TStringBuf("snippets"), TStringBuf("matchedobjects/1.x"));
    }

    for (const auto& businesssFilter : businessFilters) {
        cgi.InsertUnescaped(TStringBuf("business_filter"), businesssFilter);
    }

    MultiRequest = NHttpFetcher::MultiRequest();
    GeoMetasearchRequest = Ctx.GetSources().GeoMetaSearchResolveText().AttachRequest(MultiRequest)->AddCgiParams(cgi).Fetch();

    if (ResultIndexOnPage == PAGE_SIZE) {
        // We need to ask next page
        cgi.ReplaceUnescaped(TStringBuf("p"), ToString(pageNum + 1));
        NextPageRequest = Ctx.GetSources().GeoMetaSearchResolveTextNextPage().AttachRequest(MultiRequest)->AddCgiParams(cgi).Fetch();
    }
}

TGeoObjectResolver::TGeoObjectResolver(TContext& ctx, const TGeoPosition& ll, const TStringBuf zoom)
    : Ctx(ctx)
{
    // Reverse resolving (ll -> geo)
    TCgiParameters cgi;
    cgi.InsertEscaped("gta", "accuracy");
    cgi.InsertEscaped(TStringBuf("ms"), TStringBuf("pb"));
    cgi.InsertEscaped(TStringBuf("type"), TStringBuf("geo"));
    cgi.InsertEscaped(TStringBuf("mode"), TStringBuf("reverse"));
    if (zoom) {
        cgi.InsertEscaped(TStringBuf("geocoder_pin"), "1");
        cgi.InsertEscaped(TStringBuf("geocoder_z"), zoom);
    }
    cgi.InsertEscaped(TStringBuf("ll"), TString::Join(ToString(ll.Lon), ",", ToString(ll.Lat)));
    cgi.InsertEscaped(TStringBuf("results"), TStringBuf("1"));
    cgi.InsertEscaped(TStringBuf("lang"), ctx.Meta().Lang());
    cgi.InsertEscaped(TStringBuf("origin"), GEOSEARCH_ORIGIN);

    MultiRequest = NHttpFetcher::MultiRequest();
    GeoMetasearchRequest = Ctx.GetSources().GeoMetaSearchReverseResolve().AttachRequest(MultiRequest)->AddCgiParams(cgi).Fetch();
    ResultIndexOnPage = 1;
}

TCgiParameters PrepareParametersForOrganizationRequest(const TContext& ctx, TStringBuf ms, bool extendedInfo) {
    // Organization info in json format
    TCgiParameters cgi;
    cgi.InsertUnescaped(TStringBuf("ms"), ms);
    cgi.InsertUnescaped(TStringBuf("type"), TStringBuf("biz"));
    cgi.InsertUnescaped(TStringBuf("results"), TStringBuf("1"));
    cgi.InsertUnescaped(TStringBuf("lang"), ctx.Meta().Lang());
    cgi.InsertUnescaped(TStringBuf("origin"), GEOSEARCH_ORIGIN);
    if (extendedInfo) {
        cgi.InsertUnescaped(TStringBuf("snippets"), TStringBuf("businessrating/2.x,photos/2.x,masstransit/2.x,reviews_keyphrases"));
    } else {
        cgi.InsertUnescaped(TStringBuf("snippets"), TStringBuf("businessrating/2.x,photos/1.x"));
    }

    if (ctx.Meta().HasLocation()) {
        const auto& loc = ctx.Meta().Location();
        TStringBuilder distanceFromStr;
        distanceFromStr << loc.Lon() << ',' << loc.Lat();
        cgi.InsertUnescaped(TStringBuf("ull"), distanceFromStr);
    }

    return cgi;
}

TGeoObjectResolver::TGeoObjectResolver(TContext& ctx, TStringBuf orgId, bool extendedInfo, bool proto)
    : Ctx(ctx)
{
    TCgiParameters cgi;
    if (proto) {
        cgi = PrepareParametersForOrganizationRequest(ctx, TStringBuf("pb"), extendedInfo);
    } else {
        cgi = PrepareParametersForOrganizationRequest(ctx, TStringBuf("json"), extendedInfo);
    }
    cgi.InsertUnescaped(TStringBuf("business_oid"), orgId);

    MultiRequest = NHttpFetcher::MultiRequest();
    GeoMetasearchRequest = Ctx.GetSources().GeoMetaSearchOrganization().AttachRequest(MultiRequest)->AddCgiParams(cgi).Fetch();
    ResultIndexOnPage = 1;
}

TGeoObjectResolver::TGeoObjectResolver(TContext& ctx, const TStringBuf searchText, const TVector<TStringBuf>& orgIds)
    : Ctx(ctx)
{
    TCgiParameters cgi = PrepareParametersForOrganizationRequest(ctx, TStringBuf("json"), /*extendedInfo*/ false);
    cgi.InsertUnescaped(TStringBuf("text"), searchText);

    MultiRequest = NHttpFetcher::WeakMultiRequest();
    for (auto orgId: orgIds) {
        TCgiParameters orgCgi = cgi;
        orgCgi.InsertUnescaped(TStringBuf("business_oid"), orgId);
        MultiGeoMetasearchRequest.push_back(Ctx.GetSources().GeoMetaSearchOrganization().AttachRequest(MultiRequest)->AddCgiParams(orgCgi).Fetch());
    }
    ResultIndexOnPage = 1;
}

TResultValue TGeoObjectResolver::DoParsePage(NHttpFetcher::THandle::TRef handle, const size_t resultIndexOnPage,
     const size_t docsCount, TVector<NSc::TValue>& result, NSc::TValue* resolvedWhere)
{
    NHttpFetcher::TResponse::TRef r = handle->Wait();
    if (r->IsError()) {
        TStringBuilder errText;
        errText << TStringBuf("Fetching from geometasearch error: ") << r->GetErrorText();
        return TError(TError::EType::SYSTEM, errText);
    }

    ::yandex::maps::proto::common2::response::Response response;
    if (!response.ParseFromString(r->Data)) {
        return TError(TError::EType::SYSTEM, "Unable to parse geometasearch protobuf");
    }

    const TStringBuf privateKey = Ctx.GlobalCtx().Secrets().NavigatorKey;

    const auto& clientFeatures = Ctx.ClientFeatures();
    NAlice::ParseGeoObjectPage(response, Ctx.GlobalCtx().GeobaseLookup(), clientFeatures,
        Ctx.UserLocation(), Ctx.ContentRestrictionLevel(),
        SearchText, resultIndexOnPage, docsCount, result, privateKey,
        clientFeatures.SupportsOpenLinkSearchViewport(), clientFeatures.SupportsIntentUrls(), resolvedWhere);

    if (result.empty()) {
        LOG(DEBUG) << TStringBuf("nothing found in geometasearch for ") << SearchText << Endl;
    }

    return TResultValue();
}

TResultValue TGeoObjectResolver::WaitAndParseResponse(NSc::TValue* firstGeo, NSc::TValue* secondGeo, NSc::TValue* resolvedWhere) {
    if (firstGeo) {
        firstGeo->SetNull();
    }
    if (secondGeo) {
        secondGeo->SetNull();
    }

    TVector<NSc::TValue> objectsInfos;
    TResultValue result = WaitAndParseResponse(objectsInfos, /* docsCount */ 2, resolvedWhere);
    if (!objectsInfos.empty()) {
        *firstGeo = objectsInfos.front();
    }
    if (secondGeo && objectsInfos.size() > 1) {
        *secondGeo = objectsInfos[1];
    }
    return result;
}

TResultValue TGeoObjectResolver::WaitAndParseResponse(TVector<NSc::TValue>& results, size_t docsCount, NSc::TValue* resolvedWhere) {
    MultiRequest->WaitAll();
    if (TResultValue err = DoParsePage(GeoMetasearchRequest, ResultIndexOnPage, docsCount, results, resolvedWhere)) {
        return err;
    }
    if (NextPageRequest) {
        if (TResultValue err = DoParsePage(NextPageRequest, /* resultIndexOnPage */ 1, /* docsCount */ 1, results, resolvedWhere)) {
            return err;
        }
    }
    return TResultValue();
}

TResultValue TGeoObjectResolver::WaitAndParseGeoCoderRoadResponse(const TString& text, TString* response) {
    return NBASS::TextToRoadName(Ctx, text, response);
}

TResultValue TGeoObjectResolver::WaitAndParseOrganizationResponse(NSc::TValue* orgInfo) {
    MultiRequest->WaitAll();
    NHttpFetcher::TResponse::TRef r = GeoMetasearchRequest->Wait();
    if (r->IsError()) {
        return TError(
            TError::EType::SYSTEM,
            TStringBuilder() << TStringBuf("Fetching from geometasearch error: ") << r->GetErrorText()
        );
    }

    NSc::TValue orgResponse = NSc::TValue::FromJson(r->Data);
    if (orgResponse.Has("features")) {
        const NSc::TValue& companyMetaData = orgResponse.TrySelect("[features][0][properties][CompanyMetaData]");
        const NSc::TValue& photos = orgResponse.TrySelect("[features][0][properties][Photos]");
        const NSc::TValue& rating = orgResponse.TrySelect("[features][0][properties][BusinessRating]");
        const NSc::TValue& categories = orgResponse.TrySelect("[features][0][properties][Categories]");
        const NSc::TValue& nearbyStopsMetaData = orgResponse.TrySelect("[features][0][properties][NearbyStopsMetadata]");
        const NSc::TValue& experimentalMetaData = orgResponse.TrySelect("[features][0][properties][ExperimentalMetaData]");

        if (!companyMetaData.IsNull()) {
            (*orgInfo)["company_meta_data"] = companyMetaData;
        }
        if (!photos.IsNull()) {
            (*orgInfo)["photos"] = photos;
        }
        if (!rating.IsNull()) {
            (*orgInfo)["rating"] = rating;
        }
        if (!categories.IsNull()) {
            (*orgInfo)["categories"] = categories;
        }
        if (!nearbyStopsMetaData.IsNull()) {
            (*orgInfo)["nearby_stops_meta_data"] = nearbyStopsMetaData;
        }
        if (!experimentalMetaData.IsNull()) {
            if (experimentalMetaData.Has("Items")) {
                for (size_t i = 0; i < experimentalMetaData["Items"].ArraySize(); ++i) {
                    if (experimentalMetaData["Items"][i].Has("key") && experimentalMetaData["Items"][i]["key"] == "reviews_keyphrases") {
                        (*orgInfo)["reviews_keyphrases"] = experimentalMetaData["Items"][i]["values"];
                        break;
                    }
                }
            }
        }

    }

    return TResultValue();
}

TResultValue TGeoObjectResolver::WaitAndParseMultiOrganizationResponse(NSc::TValue* allOrgInfo) {
    MultiRequest->WaitAll();
    for (auto& geoMetasearchRequest: MultiGeoMetasearchRequest) {
        NHttpFetcher::TResponse::TRef r = geoMetasearchRequest->Wait();
        if (r->IsError()) {
            allOrgInfo->Push(NSc::TValue{});
            continue;
        }

        NSc::TValue orgInfo;
        NSc::TValue orgResponse = NSc::TValue::FromJson(r->Data);
        if (orgResponse.Has("features")) {
            const NSc::TValue& companyMetaData = orgResponse.TrySelect("[features][0][properties][CompanyMetaData]");
            const NSc::TValue& photos = orgResponse.TrySelect("[features][0][properties][Photos]");
            const NSc::TValue& rating = orgResponse.TrySelect("[features][0][properties][BusinessRating]");

            if (!companyMetaData.IsNull()) {
                orgInfo["company_meta_data"] = companyMetaData;
            }
            if (!photos.IsNull()) {
                orgInfo["photos"] = photos;
            }
            if (!rating.IsNull()) {
                orgInfo["rating"] = rating;
            }
        }
        allOrgInfo->Push(orgInfo);
    }

    return TResultValue();
}

void TGeoObjectResolver::ReplaceCountryWithCapital(TContext& ctx, NSc::TValue* geoObject) {
    if (!geoObject || geoObject->IsNull()) {
        return;
    }

    // Sometimes, geometasearch returns geoid for country,
    // but actually response was about city or street (e.g.: "Лимассол", ASSISTANT-2865)
    // So, do additional check of level and address fields
    if (GeoObjectType(*geoObject) != TStringBuf("geo") || (*geoObject)["level"].GetString() != TStringBuf("over_city")) {
        return;
    }

    static const TVector<TStringBuf> addressFields = {"house", "street", "city"};
    for (const auto& field : addressFields) {
        if (geoObject->Has(field)) {
            return;
        }
    }

    NGeobase::TId geoid = (*geoObject)["geoid"].GetIntNumber(NGeobase::UNKNOWN_REGION);
    if (!NAlice::IsValidId(geoid)) {
        return;
    }

    const auto& geobase = ctx.GlobalCtx().GeobaseLookup();
    NGeobase::TRegion region = geobase.GetRegionById(geoid);
    if (region.GetEType() != NGeobase::ERegionType::COUNTRY) {
        return;
    }

    if (NGeobase::TId capital = region.GetCapitalId(); NAlice::IsValidId(capital)) {
        (*geoObject).Clear();
        TStringBuilder addressLine;

        NGeobase::TLinguistics countryNames = geobase.GetLinguistics(geoid, ctx.MetaLocale().Lang);
        addressLine << countryNames.NominativeCase;
        (*geoObject)["country"] = countryNames.NominativeCase;

        NGeobase::TLinguistics capitalNames = geobase.GetLinguistics(capital, ctx.MetaLocale().Lang);
        addressLine << (addressLine ? ", " : "") << capitalNames.NominativeCase;
        (*geoObject)["city"] = capitalNames.NominativeCase;

        (*geoObject)["address_line"] = addressLine;

        (*geoObject)["level"] = "city";
        (*geoObject)["geoid"].SetIntNumber(capital);
        NAlice::FillCityPrepcase(geobase, capital, ctx.MetaLocale().Lang, geoObject);

        NSc::TValue& location = (*geoObject)["location"];
        NGeobase::TRegion capitalRegion = geobase.GetRegionById(capital);
        location["lon"] = capitalRegion.GetLongitude();
        location["lat"] = capitalRegion.GetLatitude();
    }
}

TMaybe<TGeoPosition> InitGeoPositionFromLocation(TContext::TMeta::TLocationConst location) {
    TMaybe<TGeoPosition> pos;

    if (location.HasLat() && location.HasLon()) {
        pos = TGeoPosition();
        pos->Lon = location.Lon();
        pos->Lat = location.Lat();
    }

    return pos;
}

} // namespace NBASS
