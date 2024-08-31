#include "geo_cgi_builder.h"

namespace NAlice {

namespace {

constexpr TStringBuf GEOSEARCH_ORIGIN = "assistant_bass";

} // namespace

TGeoObjectCgiBuilder::TGeoObjectCgiBuilder(TStringBuf msType) {
    Cgi.InsertEscaped(TStringBuf("ms"), msType);
    Cgi.InsertEscaped(TStringBuf("origin"), GEOSEARCH_ORIGIN);
}

TGeoObjectCgiBuilder& TGeoObjectCgiBuilder::AddBusinessFilters(const TVector<TStringBuf>& businessFilters) {
    for (const auto& filter : businessFilters) {
        Cgi.InsertUnescaped(TStringBuf("business_filter"), filter);
    }
    return *this;
}

TGeoObjectCgiBuilder& TGeoObjectCgiBuilder::AddExperimentalMeta() {
    Cgi.InsertEscaped(TStringBuf("gta"), TStringBuf("matchedobjects/1.x"));
    Cgi.InsertEscaped(TStringBuf("snippets"), TStringBuf("matchedobjects/1.x"));
    return *this;
}

TGeoObjectCgiBuilder& TGeoObjectCgiBuilder::AddGeoType(TStringBuf geoType) {
    Cgi.InsertEscaped(TStringBuf("type"), geoType);
    return *this;
}

TGeoObjectCgiBuilder& TGeoObjectCgiBuilder::AddGta() {
    Cgi.InsertEscaped(TStringBuf("gta"), TStringBuf("geoid"));
    Cgi.InsertEscaped(TStringBuf("gta"), TStringBuf("accuracy"));
    Cgi.InsertEscaped(TStringBuf("gta"), TStringBuf("ll"));
    return *this;
}

TGeoObjectCgiBuilder& TGeoObjectCgiBuilder::AddLang(TStringBuf lang) {
    Cgi.InsertEscaped(TStringBuf("lang"), lang);
    return *this;
}

TGeoObjectCgiBuilder& TGeoObjectCgiBuilder::AddMode(TStringBuf mode) {
    Cgi.InsertEscaped(TStringBuf("mode"), mode);
    return *this;
}

TGeoObjectCgiBuilder& TGeoObjectCgiBuilder::AddOrgId(TStringBuf businessOid) {
    Cgi.InsertEscaped(TStringBuf("business_oid"), businessOid);
    return *this;
}

TGeoObjectCgiBuilder& TGeoObjectCgiBuilder::AddPageNumber(int pageNumber) {
    Cgi.InsertEscaped(TStringBuf("p"), ToString(pageNumber));
    return *this;
}

TGeoObjectCgiBuilder& TGeoObjectCgiBuilder::AddResultsPageSize(int resultsPageSize) {
    Cgi.InsertEscaped(TStringBuf("results"), ToString(resultsPageSize));
    return *this;
}

TGeoObjectCgiBuilder& TGeoObjectCgiBuilder::AddSearchPos(const TMaybe<TGeoPosition>& searchPos) {
    if (searchPos) {
        // Specify user`s location, otherwise we may receive result in another city
        const TString& llStr = searchPos->GetLonLatString();
        Cgi.InsertEscaped(TStringBuf("ll"), llStr);
        Cgi.InsertEscaped(TStringBuf("ull"), llStr);
    }
    return *this;
}

TGeoObjectCgiBuilder& TGeoObjectCgiBuilder::AddSearchText(TStringBuf searchText) {
    Cgi.InsertEscaped(TStringBuf("text"), searchText);
    return *this;
}

TGeoObjectCgiBuilder& TGeoObjectCgiBuilder::AddSortBy(TStringBuf sortBy) {
    if (!sortBy.empty()) {
        Cgi.InsertEscaped(TStringBuf("sort"), sortBy);
    }
    return *this;
}

TGeoObjectCgiBuilder& TGeoObjectCgiBuilder::AddSnippets(TStringBuf snippets) {
    Cgi.InsertEscaped(TStringBuf("snippets"), snippets);
    return *this;
}

TGeoObjectCgiBuilder& TGeoObjectCgiBuilder::AddZoom(TStringBuf zoom) {
    if (zoom) {
        Cgi.InsertEscaped(TStringBuf("geocoder_pin"), TStringBuf("1"));
        Cgi.InsertEscaped(TStringBuf("geocoder_z"), zoom);
    }
    return *this;
}

} // namespace NAlice
