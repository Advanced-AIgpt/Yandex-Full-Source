#pragma once

#include <alice/library/util/status.h>

#include <library/cpp/geobase/lookup.hpp>
#include <library/cpp/scheme/scheme.h>

#include <util/generic/strbuf.h>

namespace NAlice {

enum class EGeoUtilErrorCode {
    INVALID_DATA /* "invalid_data" */,
};

using TGeoUtilError = TGenericError<EGeoUtilErrorCode>;
using TGeoUtilStatus = TMaybe<TGeoUtilError>;

/** Returns parent with type CITY
 * !!! If no such parent, returns given geoid !!! */
NGeobase::TId ReduceGeoIdToCity(const NGeobase::TLookup& geobase, NGeobase::TId geoId);

/** Gets infinitive and prepositional from from GeoBase
 * @param[out] name — infinitive form
 * @param[out] namePrepcase — prepositional form with prepositon
 */
void GeoIdToNames(const NGeobase::TLookup& geobase, NGeobase::TId geoId, TStringBuf lang, TString* name, TString* namePrepcase);

/** Returns appropriate TLD for region
 * default is "ru"
 */
TStringBuf GeoIdToTld(const NGeobase::TLookup& geobase, NGeobase::TId geoId);

void AddObsoleteCaseForms(const NGeobase::TLookup& geobase, NGeobase::TId geoId, TStringBuf lang, NSc::TValue* geoJson);
void AddAllCaseForms(const NGeobase::TLookup& geobase, NGeobase::TId geoId, TStringBuf lang, NSc::TValue* geoJson, bool wantObsolete = false);
void AddAllCaseFormsWithFallbackLanguage(const NGeobase::TLookup& geobase, NGeobase::TId geoId, TStringBuf lang, NSc::TValue* geoJson, bool wantObsolete = false);

void FillInUserCity(NGeobase::TId geoId, NSc::TValue* geoObject);
void FillCityPrepcase(const NGeobase::TLookup& geobase, NGeobase::TId geoId, TStringBuf lang, NSc::TValue* geoObject);

inline TStringBuf DetectGeoObjectType(const NSc::TValue& geoObject) {
    if (geoObject.Has("company_name")) {
        return TStringBuf("poi");
    }
    if (geoObject.Has("name")) {
        return TStringBuf("user_bookmark");
    }
    return TStringBuf("geo");
}

inline bool IsValidId(NGeobase::TId geoId) {
    return geoId > NGeobase::ROOT_ID;
}

// GeoId different type parsers
[[nodiscard]] TGeoUtilStatus ParseLatLon(const TStringBuf value, NGeobase::TId& geoId, const NGeobase::TLookup& geobase);
[[nodiscard]] TGeoUtilStatus ParseSysGeo(const TStringBuf value, NGeobase::TId& geoId);
[[nodiscard]] TGeoUtilStatus ParseGeoAddrAddress(const TStringBuf value, NGeobase::TId& geoId);
[[nodiscard]] TGeoUtilStatus ParseGeoId(const TStringBuf value, NGeobase::TId& geoId);

} // namespace NAlice
