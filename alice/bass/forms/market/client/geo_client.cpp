#include "geo_client.h"

#include <alice/bass/forms/market/util/report.h>

#include <alice/bass/forms/geo_resolver.h>

namespace NBASS {

namespace NMarket {

TMaybe<TSchemeHolder<TAddressScheme>> RequestAddressResolution(
    const TMarketContext& ctx,
    TStringBuf searchText,
    TMaybe<NBASS::TGeoPosition> geoPosition)
{
    using TGeoResponseItemSchemeConst = NBassApi::TGeoResponseItemConst<TBoolSchemeTraits>;

    TGeoObjectResolver resolver(ctx.Ctx(), searchText, geoPosition);
    NSc::TValue geoObj;
    auto err = resolver.WaitAndParseResponse(&geoObj);
    if (err) {
        throw THttpException(
            TStringBuilder() << TStringBuf("Geo object resolver error: ") << err->ToJson().ToJsonPretty());
    }
    if (geoObj.IsNull()) {
        return Nothing();
    }

    TSchemeHolder<TAddressScheme> address;
    TGeoResponseItemSchemeConst geoObjScheme(&geoObj);
    if (geoObjScheme.HasCountry()) {
        address->Country() = geoObjScheme.Country();
    }
    if (geoObjScheme.HasCity()) {
        address->City() = geoObjScheme.City();
    }
    if (geoObjScheme.HasCountry()) {
        address->Street() = geoObjScheme.Street();
    }
    if (geoObjScheme.HasHouse()) {
        address->House() = geoObjScheme.House();
    }
    address->Text() = geoObjScheme.AddressLine();
    address->RegionId() = geoObjScheme.Geoid();

    return MakeMaybe(address);
}

} // namespace NMarket

} // namespace NBASS
