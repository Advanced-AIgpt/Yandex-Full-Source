#include "report_client.h"

#include <alice/bass/forms/market/market_url_builder.h>
#include <alice/bass/forms/market/util/report.h>

namespace NBASS {

namespace NMarket {

const TModel& TReportDocument::Model() const
{
    Y_ASSERT(Type_ == EType::MODEL);
    if (Y_UNLIKELY(Model_.Empty())) {
        Model_ = TModel(Data.Value());
    }
    return Model_.GetRef();
}

const TOffer& TReportDocument::Offer() const
{
    Y_ASSERT(Type_ == EType::OFFER);
    if (Y_UNLIKELY(Offer_.Empty())) {
        Offer_ = TOffer(Data.Value());
    }
    return Offer_.GetRef();
}

TReportDocument::EType TReportDocument::GetType() const
{
    auto entityType = Data.Value()["entity"].GetString();
    if (entityType == TStringBuf("product")) {
        return EType::MODEL;
    } else if (entityType == TStringBuf("offer")) {
        return EType::OFFER;
    }
    return EType::NONE;
}

const TVector<TReportDocument>& TReportSearchResponse::Documents() const
{
    if (Y_UNLIKELY(Documents_.Empty())) {
        Documents_ = TVector<TReportDocument>(Reserve(Data->Search().Results().Size()));
        for (const auto& doc : Data->Search().Results()) {
            Documents_->emplace_back(doc);
        }
    };
    return Documents_.GetRef();
}

TReportClient::TReportClient(TMarketContext& ctx)
    : TBaseClient(ctx.GetSources(), ctx)
    , RegionId(ctx.UserRegion())
    , Clid(static_cast<i32>(ctx.GetMarketClid()))
    , Uuid(ctx.Meta().UUID())
    , Reqid(ctx.RequestId())
    , Ip(ctx.Meta().ClientIP())
    , BundleUrlInfo(ctx.GetExperiments().AddToCart()
        ? MakeMaybe(TBundleUrlInfo(TMarketUrlBuilder(ctx).GetBeruBundleBaseUrl(), static_cast<i32>(ctx.GetMarketClid())))
        : Nothing())
{
}

TSourceRequestFactory TReportClient::GetSource(const EMarketType& marketType)
{
    switch (marketType) {
        case EMarketType::BLUE: {
            return Sources.MarketBlue();
        }
        case EMarketType::GREEN: {
            return Sources.Market();
        }
    }
}

THttpResponse<TReportSku> TReportClient::GetSkuOffers(ui64 sku)
{
    const auto& source = Sources.MarketBlue();

    TCgiParameters cgi = CreateReportCgiParams();
    cgi.InsertUnescaped(TStringBuf("rgb"), TStringBuf("blue"));
    cgi.InsertUnescaped(TStringBuf("regional-delivery"), TStringBuf("1"));
    cgi.InsertUnescaped(TStringBuf("offer-shipping"), TStringBuf("delivery"));
    cgi.InsertUnescaped(TStringBuf("market-sku"), ToString(sku));
    cgi.InsertUnescaped(TStringBuf("place"), TStringBuf("sku_offers"));
    cgi.InsertUnescaped(TStringBuf("show-models-specs"), TStringBuf("msku-friendly"));
    if (BundleUrlInfo) {
        cgi.InsertUnescaped(TStringBuf("show-urls"), TStringBuf("cpa"));
        cgi.InsertUnescaped(
            TStringBuf("rearr-factors"),
            TStringBuilder() << "market_alice_return_bundle_urls="
                << BundleUrlInfo->BaseUrl << ',' << BundleUrlInfo->Clid);
    }

    return Run<TReportSku>(source, cgi, NSc::TValue(), CreateHeaders()).Wait();
}

TResponseHandle<TReportSearchResponse> TReportClient::GetDefaultOfferAsync(
    TModelId modelId,
    const NBASS::NMarket::TCgiGlFilters& glFilters,
    const EMarketType marketType,
    const NSc::TValue& price,
    TMaybe<ui64> hidMaybe,
    TMaybe<ui64> feshMaybe)
{
    const auto& source = GetSource(marketType);

    TCgiParameters cgi = CreateReportCgiParams();
    switch (marketType) {
        case EMarketType::BLUE: {
            cgi.InsertUnescaped(TStringBuf("rgb"), TStringBuf("blue"));
            break;
        }
        case EMarketType::GREEN: {
            cgi.InsertUnescaped(TStringBuf("rgb"), TStringBuf("green_with_blue"));
            break;
        }
    }
    cgi.InsertUnescaped(TStringBuf("regional-delivery"), TStringBuf("1"));
    cgi.InsertUnescaped(TStringBuf("offer-shipping"), TStringBuf("delivery"));
    cgi.InsertUnescaped(TStringBuf("hyperid"), ToString(modelId));
    cgi.InsertUnescaped(TStringBuf("place"), TStringBuf("defaultoffer"));
    if (BundleUrlInfo) {
        cgi.InsertUnescaped(TStringBuf("show-urls"), TStringBuf("external,encryptedmodel,cpa"));
        cgi.InsertUnescaped(
            TStringBuf("rearr-factors"),
            TStringBuilder() << "market_alice_return_bundle_urls="
                << BundleUrlInfo->BaseUrl << ',' << BundleUrlInfo->Clid);
    } else {
        cgi.InsertUnescaped(TStringBuf("show-urls"), TStringBuf("external,encryptedmodel"));
    }
    if (hidMaybe.Defined()) {
        cgi.InsertUnescaped(TStringBuf("hid"), ToString(*hidMaybe));
    }
    if (feshMaybe.Defined()) {
        cgi.InsertUnescaped(TStringBuf("fesh"), ToString(*feshMaybe));
    }
    AddGlFilters(glFilters, cgi);
    SetPriceCgiParams(price, cgi);

    return Run<TReportSearchResponse>(source, cgi, NSc::TValue(), CreateHeaders());
}

TResponseHandle<TReportSearchResponse> TReportClient::GetOfferAsync(TStringBuf wareId)
{
    const auto& source = Sources.Market();
    TCgiParameters cgi = CreateReportCgiParams();
    cgi.InsertUnescaped(TStringBuf("place"), TStringBuf("offerinfo"));
    cgi.InsertUnescaped(TStringBuf("offerid"), ToString(wareId));
    cgi.InsertUnescaped(TStringBuf("show-urls"), TStringBuf("external,cpa,showPhone,geo,geoShipping,decrypted"));
    cgi.InsertUnescaped(TStringBuf("onstock"), TStringBuf("1"));
    cgi.InsertUnescaped(TStringBuf("local-offers-first"), TStringBuf("0"));
    cgi.InsertUnescaped(TStringBuf("adult"), TStringBuf("1"));
    cgi.InsertUnescaped(TStringBuf("show-model-card-params"), TStringBuf("1"));
    return Run<TReportSearchResponse>(source, cgi, NSc::TValue(), CreateHeaders());
}

TResponseHandle<TReportSearchResponse> TReportClient::GetProductOffersAsync(
    ui64 modelId,
    const NBASS::NMarket::TCgiGlFilters& glFilters,
    const NSc::TValue& price,
    TMaybe<ui64> hidMaybe)
{
    // todo cpmdo,local-offers-first,use-default-offers?
    const auto& source = Sources.Market();
    TCgiParameters cgi = CreateReportCgiParams();
    cgi.InsertUnescaped(TStringBuf("adult"), TStringBuf("1"));
    cgi.InsertUnescaped(TStringBuf("onstock"), TStringBuf("1"));
    cgi.InsertUnescaped(TStringBuf("rgb"), TStringBuf("green_with_blue"));
    cgi.InsertUnescaped(TStringBuf("regional-delivery"), TStringBuf("1"));
    cgi.InsertUnescaped(TStringBuf("show-urls"), TStringBuf("external,encryptedmodel"));
    cgi.InsertUnescaped(TStringBuf("grhow"), TStringBuf("shop"));
    cgi.InsertUnescaped(TStringBuf("hyperid"), ToString(modelId));
    cgi.InsertUnescaped(TStringBuf("place"), TStringBuf("productoffers"));
    if (hidMaybe.Defined()) {
        cgi.InsertUnescaped(TStringBuf("hid"), ToString(*hidMaybe));
    }
    AddGlFilters(glFilters, cgi);
    SetPriceCgiParams(price, cgi);

    return Run<TReportSearchResponse>(source, cgi, NSc::TValue(), CreateHeaders());
}

TResponseHandle<TReportSearchResponse> TReportClient::GetModelInfoAsync(
    ui64 modelId,
    const NBASS::NMarket::TCgiGlFilters& glFilters,
    const TRedirectCgiParams& redirectCgiParams,
    const NSc::TValue& price,
    TMaybe<ui64> hidMaybe)
{
    // todo cpmdo,local-offers-first,use-default-offers?
    const auto& source = Sources.Market();
    TCgiParameters cgi = CreateReportCgiParams();
    cgi.InsertUnescaped(TStringBuf("adult"), TStringBuf("1"));
    cgi.InsertUnescaped(TStringBuf("onstock"), TStringBuf("1"));
    cgi.InsertUnescaped(TStringBuf("rgb"), TStringBuf("green_with_blue"));
    cgi.InsertUnescaped(TStringBuf("regional-delivery"), TStringBuf("1"));
    cgi.InsertUnescaped(TStringBuf("hyperid"), ToString(modelId));
    cgi.InsertUnescaped(TStringBuf("place"), TStringBuf("modelinfo"));
    if (hidMaybe.Defined()) {
        cgi.InsertUnescaped(TStringBuf("hid"), ToString(*hidMaybe));
    }
    AddGlFilters(glFilters, cgi);
    SetRedirectCgiParams(redirectCgiParams, cgi);
    SetPriceCgiParams(price, cgi);

    return Run<TReportSearchResponse>(source, cgi, NSc::TValue(), CreateHeaders());
}

TCgiParameters TReportClient::CreateReportCgiParams()
{
    TCgiParameters cgi;
    cgi.InsertUnescaped(TStringBuf("regset"), TStringBuf("2"));
    cgi.InsertUnescaped(TStringBuf("alice"), TStringBuf("1"));
    cgi.InsertUnescaped(TStringBuf("rids"), ToString(RegionId));
    cgi.InsertUnescaped(TStringBuf("pp"), ToString(PpCgiParam));
    cgi.InsertUnescaped(TStringBuf("bsformat"), TStringBuf("2"));
    cgi.InsertUnescaped(TStringBuf("pof"), FormatPof(Clid));
    cgi.InsertUnescaped(TStringBuf("uuid"), Uuid);
    cgi.InsertUnescaped(TStringBuf("wprid"), Reqid);
    cgi.InsertUnescaped(TStringBuf("ip"), Ip);
    cgi.InsertUnescaped(TStringBuf("allow-collapsing"), TStringBuf("1"));
    return cgi;
}

THashMap<i32, TString> TReportClient::GetAllDeliveryStatuses(const THashSet<i32>& deliveryIds) {
    const auto& source = Sources.MarketBlue();

    TCgiParameters cgi = CreateReportCgiParams();
    cgi.InsertUnescaped(TStringBuf("place"), TStringBuf("delivery_status"));
    cgi.InsertUnescaped(TStringBuf("rgb"), TStringBuf("blue"));
    for (auto deliveryStatusId: deliveryIds) {
        cgi.InsertUnescaped(TStringBuf("delivery-status-id"), ToString(deliveryStatusId));
    }
    const auto deliveryInfo = Run<TReportDeliveryInfo>(source, cgi).Wait().GetResponse();
    const auto statusesInfo = deliveryInfo->Results();
    THashMap<i32, TString> result;

    for (const auto& statusInfo: statusesInfo) {
        i32 id = statusInfo.Id();
        TStringBuf text = statusInfo.Texts().Mobile();
        result[id] = ToString(text);
    }

    return result;
}

THashMap<TString, TString> TReportClient::CreateHeaders() const
{
    THashMap<TString, TString> headers;
    return headers;
}

} // namespace NMarket

} // namespace NBASS
