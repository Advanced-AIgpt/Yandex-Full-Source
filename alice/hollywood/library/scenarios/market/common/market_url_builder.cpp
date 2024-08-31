#include "market_url_builder.h"

#include <util/string/builder.h>
#include <util/string/cast.h>
#include <util/string/join.h>

namespace NAlice::NHollywood::NMarket {

TMarketUrlBuilder::TMarketUrlBuilder(
        EClid clid,
        TStringBuf uuid,
        TStringBuf reqid,
        bool isTouch)
    : Clid(clid)
    , Uuid(ToString(uuid))
    , Reqid(ToString(reqid))
    , IsTouch(isTouch)
{}

TString TMarketUrlBuilder::GetMarketSearchUrl(
    TStringBuf text,
    TMaybe<NGeobase::TId> regionId,
    TMaybe<EMarketGoodState> goodState,
    bool redirect) const
{
    TCgiParameters cgi = GetBaseCgi(regionId);
    cgi.InsertUnescaped(TStringBuf("onstock"), TStringBuf("1"));
    cgi.InsertEscaped(TStringBuf("text"), text);
    if (goodState) {
        cgi.InsertUnescaped(TStringBuf("good-state"), ToString(*goodState));
    }
    if (redirect) {
        cgi.InsertUnescaped(TStringBuf("cvredirect"), TStringBuf("1"));
    }
    return GetMarketUrl(TStringBuilder() << "/search?" << cgi.Print());
}

TString TMarketUrlBuilder::GetMarketCategoryUrl(
    const TCategory& category,
    const TCgiGlFilters& glFilters,
    TMaybe<NGeobase::TId> regionId,
    TMaybe<EMarketGoodState> goodState,
    TStringBuf text,
    const TCgiRedirectParameters& redirectParams) const
{
    TCgiParameters cgi = GetBaseCgi(regionId);
    cgi.InsertUnescaped(TStringBuf("onstock"), TStringBuf("1"));
    if (category.HasHid()) {
        cgi.InsertUnescaped(TStringBuf("hid"), ToString(category.GetHid()));
    }
    if (goodState) {
        cgi.InsertUnescaped(TStringBuf("good-state"), ToString(*goodState));
    }
    glFilters.AddToCgi(cgi);
    redirectParams.AddToCgi(cgi);
    if (!text.empty()) {
        cgi.InsertEscaped(TStringBuf("text"), text);
    }

    TStringBuilder path;
    if (category.HasNid()) {
        path
            << TStringBuf("/catalog")
            << GetSlugWithPrefix(category.GetSlug())
            << '/'
            << category.GetNid()
            << TStringBuf("/list?");
    } else {
        path << TStringBuf("/search?");
    }
    return GetMarketUrl(path << cgi.Print());
}

TString TMarketUrlBuilder::GetMarketModelUrl(
    const TModelId modelId,
    TStringBuf slug,
    TMaybe<NGeobase::TId> regionId,
    const TCgiGlFilters& glFilters,
    const TCgiRedirectParameters& redirectParams,
    EMarketModelTab tab) const
{
    TCgiParameters cgi = GetBaseCgi(regionId);
    glFilters.AddToCgi(cgi);
    redirectParams.AddToCgi(cgi);
    return GetMarketUrl(
        TStringBuilder()
           << TStringBuf("/product")
           << GetSlugWithPrefix(slug)
           << '/'
           << modelId
           << GetModelTabPath(tab)
           << '?'
           << cgi.Print());
}

TString TMarketUrlBuilder::GetMarketOfferUrl(
    TStringBuf wareMd5,
    TStringBuf cpc,
    TMaybe<NGeobase::TId> regionId) const
{
    TCgiParameters cgi = GetBaseCgi(regionId);
    if (!cpc.empty()) {
        cgi.InsertUnescaped(TStringBuf("cpc"), cpc);
    }
    return GetMarketUrl(
        TStringBuilder() << TStringBuf("/offer/") << wareMd5 << '?' << cgi.Print());
}

TStringBuf TMarketUrlBuilder::GetBaseUrl() const
{
    return IsTouch
        ? TStringBuf("https://m.market.yandex.ru")
        : TStringBuf("https://market.yandex.ru");
}

TCgiParameters TMarketUrlBuilder::GetBaseCgi(TMaybe<NGeobase::TId> regionId) const
{
    TCgiParameters cgi;
    if (Clid != EClid::OTHER) {
        cgi.InsertUnescaped(TStringBuf("clid"), ToString(static_cast<TClidValue>(Clid)));
    }
    if (regionId.Defined()) {
        cgi.InsertUnescaped(TStringBuf("lr"), ToString(regionId));
    }
    cgi.InsertUnescaped(TStringBuf("uuid"), Uuid);
    cgi.InsertUnescaped(TStringBuf("wprid"), Reqid);
    cgi.InsertUnescaped(TStringBuf("adult"), TStringBuf("0"));
    return cgi;
}

TString TMarketUrlBuilder::GetSlugWithPrefix(TStringBuf slug)
{
    if (slug.Empty()) {
        return TString("");
    }
    return TStringBuilder() << TStringBuf("--") << slug;
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

TString TMarketUrlBuilder::GetMarketUrl(TStringBuf uri) const
{
    const TStringBuf url = GetBaseUrl();
    TStringBuilder urlBuilder;
    urlBuilder << url;
    if (!uri.empty()) {
        urlBuilder << uri;
    }
    return urlBuilder;
}

TMarketUrlBuilder CreateMarketUrlBuilder(EClid clid, const TScenarioBaseRequestWrapper& request)
{
    return {
        clid,
        request.ClientInfo().Uuid,
        request.RequestId(),
        request.ClientInfo().IsTouch(),
    };
}

} // namespace NAlice::NHollywood::NMarket
