#pragma once

#include "context.h"
#include "types.h"

#include <alice/bass/forms/context/fwd.h>

#include <util/generic/hash.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <library/cpp/cgiparam/cgiparam.h>

namespace NBASS {

namespace NMarket {

class TMarketUrlBuilder {
public:
    enum EMarketModelTab {
        Main,
        Offers,
        Reviews,
    };
    static const i32 INESSENTIAL_CLID = 0;

    TMarketUrlBuilder();
    TMarketUrlBuilder(const TMarketContext& ctx);

    TMarketUrlBuilder(const TMarketUrlBuilder&) = default;

    TString GetMarketUrl(
        EMarketType marketType = DEFAULT_MARKET_TYPE,
        TStringBuf uri = TStringBuf("")) const;

    TString GetMarketSearchUrl(
        EMarketType marketType,
        TStringBuf text,
        TMaybe<ui64> regionId = Nothing(),
        TMaybe<EMarketGoodState> goodState = Nothing(),
        bool redirect = false,
        ui32 galleryNumber = 0,
        double priceFrom = -1,
        double priceTo = -1) const;

    TString GetMarketCategoryUrl(
        EMarketType marketType,
        const TCategory& category,
        const THashMap<TString, TVector<TString>>& glFilters,
        TMaybe<ui64> regionId = Nothing(),
        TMaybe<EMarketGoodState> goodState = Nothing(),
        TStringBuf text = "",
        const TRedirectCgiParams& redirectParams = TRedirectCgiParams(),
        ui32 galleryNumber = 0,
        double priceFrom = -1,
        double priceTo = -1,
        TStringBuf suggestTest = TStringBuf("")) const;

    TString GetMarketModelUrl(
        TModelId,
        TStringBuf slug,
        i64 regionId,
        ui32 galleryNumber,
        ui32 galleryPosition,
        const TCgiGlFilters& glFilters = TCgiGlFilters(),
        double priceFrom = -1,
        double priceTo = -1,
        const TRedirectCgiParams& redirectParams = TRedirectCgiParams(),
        EMarketModelTab tab = EMarketModelTab::Main) const;

    TString GetMarketOfferUrl(
        TStringBuf ware_md5,
        TStringBuf cpc,
        i64 regionId,
        ui32 galleryNumber,
        ui32 galleryPosition) const;

    TString GetPictureUrl(TStringBuf picture) const;
    TString GetBeruOrderUrl(ui64 orderId) const;
    TString GetBeruCheckoutUrl(TStringBuf objId, TStringBuf feeShow, ui64 price) const;
    TString GetBeruTermsOfUseUrl() const;
    TString GetBeruModelUrl(
        TStringBuf marketSku,
        TStringBuf slug,
        ui64 regionId,
        ui32 galleryNumber,
        ui32 galleryPosition) const;
    TString GetBeruSupplierUrl(TStringBuf wareId) const;

    i32 GetClid() const;
    TString GetBeruBundleBaseUrl() const;
    TString GetClickUrl(TStringBuf clickUri) const;
    TStringBuf GetRatingIconUrl(float rating) const;
    TStringBuf GetBeruUrl(TMaybe<bool> isTouch = Nothing()) const;

private:
    TStringBuf GetBaseUrl(EMarketType marketType = DEFAULT_MARKET_TYPE, TMaybe<bool> forceIsTouch = Nothing()) const;
    TString GetTurboUrl(TStringBuf url) const;

    EScenarioType ScenarioType;
    EMarketType MarketChoiceType;
    EClids Clid;
    TStringBuf Uuid;
    TStringBuf Reqid;
    bool UseTestingUrls;
    bool IsTouch;
    bool EnableTurbo;

    TCgiParameters GetBaseCgi(TMaybe<ui64> regionId, ui32 galleryNumber = 0, ui32 galleryPosition = 0) const;
    static TString GetSlugWithPrefix(TStringBuf slug, EMarketType marketType);
    static TStringBuf GetModelTabPath(EMarketModelTab tab);
    static void SetPriceCgiParams(double priceFrom, double priceTo, TCgiParameters& cgi);
    static void SetGoodStateCgiParam(TMaybe<EMarketGoodState> state, EMarketType marketType, TCgiParameters& cgi);
};

} // namespace NMarket

} // namespace NBASS
