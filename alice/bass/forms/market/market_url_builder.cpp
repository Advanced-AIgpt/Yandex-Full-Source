#include "market_url_builder.h"

#include "util/report.h"

#include <alice/bass/forms/context/context.h>

#include <util/string/builder.h>
#include <util/string/cast.h>
#include <util/string/join.h>

namespace NBASS {

namespace NMarket {

TMarketUrlBuilder::TMarketUrlBuilder()
    : ScenarioType(DEFAULT_SCENARIO_TYPE)
    , MarketChoiceType(DEFAULT_MARKET_TYPE)
    , Clid(EClids::HOW_MUCH)
    , UseTestingUrls(false)
    , IsTouch(false)
    , EnableTurbo(false)
{
}

TMarketUrlBuilder::TMarketUrlBuilder(const TMarketContext& ctx)
    : ScenarioType(ctx.GetScenarioType())
    , MarketChoiceType(ctx.GetChoiceMarketType())
    , Clid(ctx.GetMarketClid())
    , Uuid(ctx.Meta().UUID())
    , Reqid(ctx.Meta().RequestId())
    , UseTestingUrls(ctx.Ctx().GetConfig().HasMarket() && ctx.Ctx().GetConfig().Market().UseTestingUrls())
    , IsTouch(ctx.MetaClientInfo().IsTouch())
    , EnableTurbo(true)
{
}

TCgiParameters TMarketUrlBuilder::GetBaseCgi(TMaybe<ui64> regionId, ui32 galleryNumber, ui32 galleryPosition) const
{
    TCgiParameters cgi;
    if (GetClid() != 0) {
        cgi.InsertUnescaped(TStringBuf("clid"), ToString(GetClid()));
    }
    if (regionId.Defined()) {
        cgi.InsertUnescaped(TStringBuf("lr"), ToString(regionId));
    }
    cgi.InsertUnescaped(TStringBuf("uuid"), Uuid);
    cgi.InsertUnescaped(TStringBuf("wprid"), Reqid);
    cgi.InsertUnescaped(TStringBuf("adult"), TStringBuf("0"));
    if (galleryNumber) {
        cgi.InsertUnescaped(TStringBuf("gallery_number"), ToString(galleryNumber));
    }
    if (galleryPosition) {
        cgi.InsertUnescaped(TStringBuf("gallery_position"), ToString(galleryPosition));
    }
    return cgi;
}

TString TMarketUrlBuilder::GetSlugWithPrefix(TStringBuf slug, EMarketType marketType)
{
    if (slug.Empty()) {
        return TString("");
    }
    switch (marketType) {
        case EMarketType::GREEN: {
            return TStringBuilder() << TStringBuf("--") << slug;
        }
        case EMarketType::BLUE: {
            return TStringBuilder() << '/' << slug;
        }
    }
}

TString TMarketUrlBuilder::GetMarketSearchUrl(
    EMarketType marketType,
    TStringBuf text,
    TMaybe<ui64> regionId,
    TMaybe<EMarketGoodState> goodState,
    bool redirect,
    ui32 galleryNumber,
    double priceFrom,
    double priceTo) const
{
    TCgiParameters cgi = GetBaseCgi(regionId, galleryNumber);
    cgi.InsertUnescaped(TStringBuf("onstock"), TStringBuf("1"));
    cgi.InsertEscaped(TStringBuf("text"), text);
    SetGoodStateCgiParam(goodState, marketType, cgi);
    if (redirect) {
        cgi.InsertUnescaped(TStringBuf("cvredirect"), TStringBuf("1"));
    }
    SetPriceCgiParams(priceFrom, priceTo, cgi);
    return GetMarketUrl(marketType, TStringBuilder() << "/search?" << cgi.Print());
}

TString TMarketUrlBuilder::GetMarketCategoryUrl(
    EMarketType marketType,
    const TCategory& category,
    const TCgiGlFilters& glFilters,
    TMaybe<ui64> regionId,
    TMaybe<EMarketGoodState> goodState,
    TStringBuf text,
    const TRedirectCgiParams& redirectParams,
    ui32 galleryNumber,
    double priceFrom,
    double priceTo,
    TStringBuf suggestText) const
{
    if (category.GetSlug().Empty()) {
        LOG(ERR)
            << TStringBuf("Empty slug for category nid=")
            << category.GetNid()
            << TStringBuf(" hid=")
            << category.GetHid()
            << Endl;
    }
    TCgiParameters cgi = GetBaseCgi(regionId, galleryNumber);
    cgi.InsertUnescaped(TStringBuf("onstock"), TStringBuf("1"));
    cgi.InsertUnescaped(TStringBuf("hid"), ToString(category.GetHid()));
    SetGoodStateCgiParam(goodState, marketType, cgi);
    SetRedirectCgiParams(redirectParams, cgi);
    AddGlFilters(glFilters, cgi);
    if (!text.empty()) {
        cgi.InsertEscaped(TStringBuf("text"), text);
    }
    SetPriceCgiParams(priceFrom, priceTo, cgi);
    if (!suggestText.empty()) {
        cgi.InsertEscaped(TStringBuf("suggest_text"), suggestText);
    }

    TStringBuilder path;
    if (category.DoesNidExist()) {
        path
            << TStringBuf("/catalog")
            << GetSlugWithPrefix(category.GetSlug(), marketType)
            << '/'
            << category.GetNid()
            << TStringBuf("/list?");
    } else {
        path << TStringBuf("/search?");
    }
    return GetMarketUrl(marketType, path << cgi.Print());
}

TString TMarketUrlBuilder::GetMarketUrl(EMarketType marketType, TStringBuf uri) const
{
    const TStringBuf url = GetBaseUrl(marketType);
    TStringBuilder urlBuilder;
    urlBuilder << url;
    if (!uri.empty()) {
        urlBuilder << uri;
    }
    return urlBuilder;
}

TString TMarketUrlBuilder::GetMarketModelUrl(
    const TModelId modelId,
    TStringBuf slug,
    i64 regionId,
    ui32 galleryNumber,
    ui32 galleryPosition,
    const TCgiGlFilters& glFilters,
    double priceFrom,
    double priceTo,
    const TRedirectCgiParams& redirectParams,
    EMarketModelTab tab) const
{
    if (slug.Empty()) {
        LOG(ERR) << TStringBuf("Empty slug for model id=") << modelId << Endl;
    }
    TCgiParameters cgi = GetBaseCgi(regionId, galleryNumber, galleryPosition);
    SetRedirectCgiParams(redirectParams, cgi);
    SetPriceCgiParams(priceFrom, priceTo, cgi);
    AddGlFilters(glFilters, cgi);
    return GetMarketUrl(
        DEFAULT_MARKET_TYPE,
        TStringBuilder()
           << TStringBuf("/product")
           << GetSlugWithPrefix(slug, MarketChoiceType)
           << '/'
           << modelId
           << GetModelTabPath(tab)
           << '?'
           << cgi.Print());
}

TString TMarketUrlBuilder::GetMarketOfferUrl(
    TStringBuf wareMd5,
    TStringBuf cpc,
    i64 regionId,
    ui32 galleryNumber,
    ui32 galleryPosition) const
{
    TCgiParameters cgi = GetBaseCgi(regionId, galleryNumber, galleryPosition);
    if (!cpc.empty()) {
        cgi.InsertUnescaped(TStringBuf("cpc"), cpc);
    }
    return GetMarketUrl(DEFAULT_MARKET_TYPE, TStringBuilder() << TStringBuf("/offer/") << wareMd5 << '?' << cgi.Print());
}

TString TMarketUrlBuilder::GetPictureUrl(TStringBuf picture) const
{
    if (picture.empty()) {
        return "http://yastatic.net/market-export/_/i/desktop/big-box.png";
    }
    return TStringBuilder() << TStringBuf("http:") << picture;
}

TString TMarketUrlBuilder::GetBeruOrderUrl(ui64 orderId) const
{
    return TStringBuilder()
        << GetBeruUrl()
        << TStringBuf("/my/order/")
        << orderId;
}

TString TMarketUrlBuilder::GetBeruCheckoutUrl(TStringBuf objId, TStringBuf feeShow, ui64 price) const
{
    TCgiParameters cgi;
    cgi.InsertUnescaped(
        TStringBuf("schema"),
        TStringBuf("objId:feeShow:buyerPrice:type:count:promoKey:cartId"));
    cgi.InsertUnescaped(
        TStringBuf("offer"),
        TStringBuilder() << objId << ':' << feeShow << ':' << price
                         << TStringBuf(":o:1::"));
    return TStringBuilder()
        << GetBeruUrl()
        << TStringBuf("/checkout?")
        << cgi.Print();
}

TString TMarketUrlBuilder::GetBeruTermsOfUseUrl() const
{
    static const TString url = "https://yandex.ru/legal/marketplace_termsofuse/";
    return url;
}

TString TMarketUrlBuilder::GetBeruModelUrl(
    TStringBuf marketSku,
    TStringBuf slug,
    ui64 regionId,
    ui32 galleryNumber,
    ui32 galleryPosition) const
{
    if (slug.Empty()) {
        LOG(ERR) << TStringBuf("Empty slug for sku id=") << marketSku << Endl;
    }
    TCgiParameters cgi = GetBaseCgi(regionId, galleryNumber, galleryPosition);
    return GetTurboUrl(TStringBuilder()
        << GetBeruUrl()
        << TStringBuf("/product")
        << GetSlugWithPrefix(slug, EMarketType::BLUE)
        << '/'
        << marketSku
        << '?'
        << cgi.Print());
}

TString TMarketUrlBuilder::GetBeruSupplierUrl(TStringBuf wareId) const
{
    TCgiParameters cgi;
    cgi.InsertUnescaped(TStringBuf("offerId"), wareId);
    cgi.InsertUnescaped(TStringBuf("clid"), ToString(GetClid()));
    return TStringBuilder()
        << GetBeruUrl(false /* itTouch */)
        << TStringBuf("/suppliers/info-by-offers?")
        << cgi.Print();
}

i32 TMarketUrlBuilder::GetClid() const
{
    return static_cast<i32>(Clid);
}

TString TMarketUrlBuilder::GetBeruBundleBaseUrl() const
{
    return ToString(GetBeruUrl(false /* forceIsTouch */));
}

TString TMarketUrlBuilder::GetClickUrl(TStringBuf clickUri) const
{
    TStringBuilder url;
    if (UseTestingUrls) {
        url << TStringBuf("https://market-click2-testing.yandex.ru");
    } else {
        url << TStringBuf("https://market-click2.yandex.ru");
    }
    url << clickUri;
    return url;
}

TStringBuf TMarketUrlBuilder::GetRatingIconUrl(float rating) const
{
    static const THashMap<float, TStringBuf> ratingIcons {
        {1, TStringBuf("http://avatars.mds.yandex.net/get-mpic/1767151/rating-1-0/1")},
        {1.5, TStringBuf("http://avatars.mds.yandex.net/get-mpic/1911047/rating-1-5/1")},
        {2, TStringBuf("http://avatars.mds.yandex.net/get-mpic/2008455/rating-2-0/1")},
        {2.5, TStringBuf("http://avatars.mds.yandex.net/get-mpic/1866164/rating-2-5/1")},
        {3, TStringBuf("http://avatars.mds.yandex.net/get-mpic/1417902/rating-3-0/1")},
        {3.5, TStringBuf("http://avatars.mds.yandex.net/get-mpic/1704691/rating-3-5/1")},
        {4, TStringBuf("http://avatars.mds.yandex.net/get-mpic/1543318/rating-4-0/1")},
        {4.5, TStringBuf("http://avatars.mds.yandex.net/get-mpic/1544149/rating-4-5/1")},
        {5, TStringBuf("http://avatars.mds.yandex.net/get-mpic/1865723/rating-5-0/1")},
    };
    if (ratingIcons.contains(rating)) {
        return ratingIcons.at(rating);
    }
    return TStringBuf("");
}

TStringBuf TMarketUrlBuilder::GetBaseUrl(EMarketType type, TMaybe<bool> forceIsTouch) const
{
    switch (type) {
        case EMarketType::GREEN: {
            return (forceIsTouch.GetOrElse(IsTouch)
                ? TStringBuf("https://m.market.yandex.ru")
                : TStringBuf("https://market.yandex.ru"));
        }
        case EMarketType::BLUE: {
            return UseTestingUrls
                ? (forceIsTouch.GetOrElse(IsTouch)
                    ? TStringBuf("https://touch.pokupki.market.fslb.yandex.ru")
                    : TStringBuf("https://desktop.pokupki.market.fslb.yandex.ru"))
                : (forceIsTouch.GetOrElse(IsTouch)
                    ? TStringBuf("https://m.pokupki.market.yandex.ru")
                    : TStringBuf("https://pokupki.market.yandex.ru"));
        }
    }
}

TStringBuf TMarketUrlBuilder::GetBeruUrl(TMaybe<bool> forceIsTouch) const
{
    return GetBaseUrl(EMarketType::BLUE, forceIsTouch);
}

TString TMarketUrlBuilder::GetTurboUrl(TStringBuf url) const
{
    if (EnableTurbo && IsTouch) {
        TCgiParameters cgi;
        cgi.InsertUnescaped(TStringBuf("text"), url);
        return TStringBuilder() << TStringBuf("https://yandex.ru/turbo?") << cgi.QuotedPrint("");
    }
    return ToString(url);
}

TStringBuf TMarketUrlBuilder::GetModelTabPath(EMarketModelTab tab)
{
    switch (tab) {
        case EMarketModelTab::Main:
            return TStringBuf("");
        case EMarketModelTab::Offers:
            return TStringBuf("/offers");
        case EMarketModelTab::Reviews:
            return TStringBuf("/reviews");
    }
}

void TMarketUrlBuilder::SetPriceCgiParams(double priceFrom, double priceTo, TCgiParameters &cgi)
{
    if (priceFrom >= 0) {
        cgi.InsertUnescaped(TStringBuf("pricefrom"), ToString(priceFrom));
    }
    if (priceTo >= 0) {
        cgi.InsertUnescaped(TStringBuf("priceto"), ToString(priceTo));
    }
}

void TMarketUrlBuilder::SetGoodStateCgiParam(TMaybe<EMarketGoodState> state, EMarketType marketType, TCgiParameters& cgi)
{
    if (state.Defined() && (marketType == EMarketType::GREEN)) {
        cgi.InsertUnescaped(TStringBuf("good-state"), ToString(*state));
    }
}

} // namespace NMarket

} // namespace NBASS
