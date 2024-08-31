#pragma once

#include <alice/library/geo_resolver/geo_position.h>

#include <library/cpp/cgiparam/cgiparam.h>

#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>

namespace NAlice {

class TGeoObjectCgiBuilder {
public:
    explicit TGeoObjectCgiBuilder(TStringBuf msType);

    TGeoObjectCgiBuilder& AddBusinessFilters(const TVector<TStringBuf>& businessFilters);
    TGeoObjectCgiBuilder& AddExperimentalMeta();
    TGeoObjectCgiBuilder& AddGeoType(TStringBuf geoType);
    TGeoObjectCgiBuilder& AddGta();
    TGeoObjectCgiBuilder& AddLang(TStringBuf lang);
    TGeoObjectCgiBuilder& AddMode(TStringBuf mode);
    TGeoObjectCgiBuilder& AddMsType(TStringBuf msType);
    TGeoObjectCgiBuilder& AddOrgId(TStringBuf businessOid);
    TGeoObjectCgiBuilder& AddPageNumber(int pageNumber);
    TGeoObjectCgiBuilder& AddResultsPageSize(int resultsPageSize);
    TGeoObjectCgiBuilder& AddSearchPos(const TMaybe<TGeoPosition>& searchPos);
    TGeoObjectCgiBuilder& AddSearchText(TStringBuf searchText);
    TGeoObjectCgiBuilder& AddSnippets(TStringBuf snippets);
    TGeoObjectCgiBuilder& AddSortBy(TStringBuf sortBy);
    TGeoObjectCgiBuilder& AddZoom(TStringBuf zoom);

    const TCgiParameters& Get() const {
        return Cgi;
    }

private:
    TCgiParameters Cgi;
};

} // namespace NAlice
