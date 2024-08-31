#include "geodb.h"

#include <alice/library/json/json.h>
#include <alice/protos/data/lat_lon.pb.h>

#include <kernel/geodb/countries.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/resource/resource.h>

#include <util/generic/vector.h>
#include <util/stream/str.h>
#include <util/string/ascii.h>
#include <util/string/builder.h>

namespace NAlice {

namespace {
using TCountryTldList = TVector<std::pair<const NGeobase::TId, TStringBuf>>;

// country ids vs tld
const TCountryTldList CountryTldList{{
    { NGeoDB::BELARUS_ID, TStringBuf("by") },
    { NGeoDB::KAZAKHSTAN_ID, TStringBuf("kz") },
    { NGeoDB::RUSSIA_ID, TStringBuf("ru") },
    { NGeoDB::TURKEY_ID, TStringBuf("com.tr") },
    { NGeoDB::UKRAINE_ID, TStringBuf("ua") },
}};


void AddObsoleteCaseForms(const NGeobase::TLinguistics& names, NSc::TValue* geoJson) {
    (*geoJson)["city"].SetString(names.NominativeCase);
    TStringBuilder prepcase;
    if (!names.Preposition.empty()) {
        prepcase << names.Preposition << ' ';
    }
    prepcase << names.PrepositionalCase;
    (*geoJson)["city_prepcase"].SetString(prepcase);
}

void FillArabicPrepositionalLinguistics(NGeobase::TLinguistics& names) {
    if (names.PrepositionalCase.empty()) {
        // City/Country/Region names are not changed in prepositional case
        names.PrepositionalCase = names.NominativeCase;
    }
    // preposition without PrepositionalCase is useless, but it makes 'city_prepcase' fakely non-empty
    if (!names.PrepositionalCase.empty()) {
        if (names.Preposition.empty()) {
            // in arabic we always use 'in'
            names.Preposition = "في";
        }
    }
}

NGeobase::TLinguistics GetLinguistics(const NGeobase::TLookup& geobase, NGeobase::TId geoId, TStringBuf lang) {
    auto result = geobase.GetLinguistics(geoId, lang.data());

    static constexpr TStringBuf ArabicLanguage = "ar";
    if (lang == ArabicLanguage) {
        FillArabicPrepositionalLinguistics(result);
    }

    return result;
}

} // namespace

NGeobase::TId ReduceGeoIdToCity(const NGeobase::TLookup& geobase, NGeobase::TId geoId) {
    if (!IsValidId(geoId)) {
        return geoId;
    }
    NGeobase::TId cityId = geobase.GetParentIdWithType(geoId, static_cast<int>(NGeobase::ERegionType::CITY));
    if (IsValidId(cityId)) {
        return cityId;
    }

    cityId = geobase.GetParentIdWithType(geoId, static_cast<int>(NGeobase::ERegionType::VILLAGE));
    return IsValidId(cityId) ? cityId : geoId;
}

void GeoIdToNames(const NGeobase::TLookup& geobase, NGeobase::TId geoId, TStringBuf lang, TString* name, TString* namePrepcase) {
    if (!IsValidId(geoId)) {
        return;
    }

    NGeobase::TLinguistics names = GetLinguistics(geobase, geoId, lang.data());
    if (name) {
        *name = names.NominativeCase;
    }
    if (namePrepcase) {
        *namePrepcase = (names.Preposition.empty() ? "" : names.Preposition + ' ') + names.PrepositionalCase;
    }
}

TStringBuf GeoIdToTld(const NGeobase::TLookup& geobase, NGeobase::TId geoId) {
    // when no region found we fallback to moscow (213)
    if (!IsValidId(geoId)) {
        geoId = NGeoDB::MOSCOW_ID;
    }

    TStringBuf tld("ru");
    for (const auto& country : CountryTldList) {
        if (geobase.IsIdInRegion(geoId, country.first)) {
            tld = country.second;
            break;
        }
    }

    return tld;
}

void AddObsoleteCaseForms(const NGeobase::TLookup& geobase, NGeobase::TId geoId, TStringBuf lang, NSc::TValue* geoJson) {
    if (!IsValidId(geoId)) {
        return;
    }

    NGeobase::TLinguistics names = GetLinguistics(geobase, geoId, lang.data());
    AddObsoleteCaseForms(names, geoJson);
}

bool AddAllCaseFormsImpl(const NGeobase::TLookup& geobase, NGeobase::TId geoId, TStringBuf lang, NSc::TValue* geoJson, bool wantObsolete) {
    if (!IsValidId(geoId)) {
        return false;
    }

    NGeobase::TLinguistics names = GetLinguistics(geobase, geoId, lang.data());

    NSc::TValue& cases = (*geoJson)["city_cases"].SetDict();

#define ADD_CASE(CASE1, CASE2) \
    if (!names.CASE1.empty()) \
        cases[#CASE2].SetString(names.CASE1);

    ADD_CASE(NominativeCase, nominative);
    ADD_CASE(GenitiveCase, genitive);
    ADD_CASE(DativeCase, dative);
    ADD_CASE(LocativeCase, locative);
    ADD_CASE(DirectionalCase, directional);
    ADD_CASE(PrepositionalCase, prepositional);
    ADD_CASE(Preposition, preposition);

#undef ADD_CASE

    if (wantObsolete) {
        AddObsoleteCaseForms(names, geoJson);
    }

    return !names.NominativeCase.empty();
}

void AddAllCaseForms(const NGeobase::TLookup& geobase, NGeobase::TId geoId, TStringBuf lang, NSc::TValue* geoJson, bool wantObsolete) {
    AddAllCaseFormsImpl(geobase, geoId, lang, geoJson, wantObsolete);
}

void AddAllCaseFormsWithFallbackLanguage(const NGeobase::TLookup& geobase, NGeobase::TId geoId, TStringBuf lang, NSc::TValue* geoJson, bool wantObsolete) {
    static const auto FallbackLanguages = THashMap<TStringBuf, TStringBuf>{
        {"ar", "en"},
    };

    if (AddAllCaseFormsImpl(geobase, geoId, lang, geoJson, wantObsolete)) {
        return;
    }

    if (const auto fallbackLang = FallbackLanguages.Value(lang, TStringBuf())) {
        AddAllCaseForms(geobase, geoId, fallbackLang, geoJson, wantObsolete);
    }
}

void FillInUserCity(NGeobase::TId userGeoId, NSc::TValue* geoObject) {
    if (!geoObject || geoObject->IsNull()) {
        return;
    }

    NSc::TValue& geo = DetectGeoObjectType(*geoObject) == TStringBuf("poi") ? (*geoObject)["geo"] : *geoObject;
    NGeobase::TId geoId = geo["geoid"].GetIntNumber(NGeobase::UNKNOWN_REGION);
    if (IsValidId(userGeoId) && IsValidId(geoId)) {
        // We reduce geoids to city, so simple comparison is OK here
        geo["in_user_city"].SetBool(userGeoId == geoId);
    }
}

void FillCityPrepcase(const NGeobase::TLookup& geobase, NGeobase::TId geoId, TStringBuf lang, NSc::TValue* geoObject) {
    const bool hasCityField = (*geoObject).Has("city");
    const bool isCity = geobase.GetERegionType(geoId) == NGeobase::ERegionType::CITY;

    if (!hasCityField && !isCity) {
        return;
    }

    TString city, cityPrepcase;
    GeoIdToNames(geobase, geoId, lang, &city, &cityPrepcase);

    // In some strange cases geoid may be from other level, so check city string
    // geometasearch returns "село NN" as city name when geobase contains just "NN" (ASSISTANT-1145)
    if (hasCityField && !(*geoObject)["city"].GetString().EndsWith(city)) {
        return;
    }

    if (!hasCityField) {
        (*geoObject)["city"] = city;
    }
    (*geoObject)["city_prepcase"] = cityPrepcase;
    AddAllCaseForms(geobase, geoId, lang, geoObject);
}

[[nodiscard]] TGeoUtilStatus ParseLatLon(const TStringBuf value, NGeobase::TId& geoId, const NGeobase::TLookup& geobase) {
    NJson::TJsonValue json;
    if (!NJson::ReadJsonTree(value, &json, /* throwOnError = */ false)) {
        return TGeoUtilError{EGeoUtilErrorCode::INVALID_DATA} << "Slot is invalid: " << value;
    }

    const auto latLon = JsonToProto<NData::TLatLon>(json, /* validateUtf8 = */ false, /* ignoreUnknownFields = */ true);
    geoId = geobase.GetRegionIdByLocation(latLon.GetLatitude(), latLon.GetLongitude());
    return {};
}

[[nodiscard]] TGeoUtilStatus ParseSysGeo(const TStringBuf value, NGeobase::TId& geoId) {
    size_t begin = FindIndexIf(value, [](const char c) { return IsAsciiDigit(c); });
    if (begin == TStringBuf::npos) {
        return TGeoUtilError{EGeoUtilErrorCode::INVALID_DATA} << "Slot is invalid: " << value;
    }
    size_t end = FindIndexIf(value.substr(begin), [](const char c) { return !IsAsciiDigit(c); });

    try {
        geoId = FromString<NGeobase::TId>(value.substr(begin, end));
        if (!NAlice::IsValidId(geoId)) {
            return TGeoUtilError{EGeoUtilErrorCode::INVALID_DATA} << "Slot is invalid: " << value;
        }
    } catch (const TFromStringException& e) {
        return TGeoUtilError{EGeoUtilErrorCode::INVALID_DATA} << e.what();
    }
    return {};
}

[[nodiscard]] TGeoUtilStatus ParseGeoAddrAddress(const TStringBuf value, NGeobase::TId& geoId) {
    NJson::TJsonValue json;
    if (!NJson::ReadJsonTree(value, &json, /* throwOnError = */ false)) {
        return TGeoUtilError{EGeoUtilErrorCode::INVALID_DATA} << "Slot is invalid: " << value;
    }

    for (const auto& key : {"BestGeoId", "BestInheritedId"}) {
        if (!json.Has(key)) {
            continue;
        }
        const NJson::TJsonValue& value = json[key];
        if (value.IsInteger()) {
            geoId = value.GetInteger();
            if (IsValidId(geoId)) {
                return {};
            }
        }
    }
    return TGeoUtilError{EGeoUtilErrorCode::INVALID_DATA} << "Slot is invalid: " << value;
}

[[nodiscard]] TGeoUtilStatus ParseGeoId(const TStringBuf value, NGeobase::TId& geoId) {
    try {
        geoId = FromString<NGeobase::TId>(value);
        if (!NAlice::IsValidId(geoId)) {
            return TGeoUtilError{EGeoUtilErrorCode::INVALID_DATA} << "Slot is invalid: " << value;
        }
    } catch (const TFromStringException& e) {
        return TGeoUtilError{EGeoUtilErrorCode::INVALID_DATA} << e.what();
    }
    return {};
}

} // namespace NAlice
