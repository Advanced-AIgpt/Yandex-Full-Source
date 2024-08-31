#pragma once

#include "bool_scheme_traits.h"
#include "base_client.h"
#include <alice/bass/forms/market/checkout_user_info.h>
#include <alice/bass/forms/market/client/checkout.sc.h>
#include <alice/bass/forms/market/client/report.sc.h>
#include <alice/bass/forms/market/types.h>

#include <library/cpp/scheme/domscheme_traits.h>

namespace NBASS {

namespace NMarket {

using TReportSku = TSchemeHolder<NBassApi::TReportSku<TBoolSchemeTraits>>;
using TReportDefaultOfferBlue = TSchemeHolder<NBassApi::TReportDefaultOfferBlue<TBoolSchemeTraits>>;
using TReportOfferConst = TSchemeHolder<NBassApi::TReportDocumentConst<TBoolSchemeTraits>>;
using TReportDocumentSchemeConst = NBassApi::TReportDocumentConst<TBoolSchemeTraits>;
using TReportFilterSchemeConst = NBassApi::TReportFilterConst<TBoolSchemeTraits>;
using TReportDeliveryInfo = TSchemeHolder<NBassApi::TDeliveryInfo<TBoolSchemeTraits>>;

class TReportDocument {
public:
    using TSchemeConst = NBassApi::TReportDocumentConst<TBoolSchemeTraits>;
    enum class EType {
        MODEL,
        OFFER,
        NONE
    };

    TReportDocument(const NSc::TValue& data)
        : Data(data)
        , Type_(GetType())
    {}

    TReportDocument(TSchemeConst scheme)
        : TReportDocument(*scheme.GetRawValue())
    {}

    const TSchemeConst& Scheme() const
    {
        return Data.Scheme();
    }

    const TSchemeConst& operator->() const
    {
        return Data.Scheme();
    }

    EType Type() const
    {
        return Type_;
    }

    const TModel& Model() const;
    const TOffer& Offer() const;

private:
    TSchemeHolder<TSchemeConst> Data;
    EType Type_;
    mutable TMaybe<TModel> Model_;
    mutable TMaybe<TOffer> Offer_;

    EType GetType() const;
};


class TReportSearchResponse {
public:
    using TSchemeConst = NBassApi::TReportSearchResponseConst<TBoolSchemeTraits>;

    TReportSearchResponse(const NSc::TValue& data)
        : Data(data)
    {
    }

    const TVector<TReportDocument>& Documents() const;

    TSchemeConst Scheme() const
    {
        return Data.Scheme();
    }

    const TSchemeConst& operator->() const {
        return Data.Scheme();
    }

private:
    TSchemeHolder<TSchemeConst> Data;
    mutable TMaybe<TVector<TReportDocument>> Documents_;
};

class TReportClient: public TBaseClient {
public:
    explicit TReportClient(TMarketContext& context);

    THttpResponse<TReportSku> GetSkuOffers(ui64 sku);

    TResponseHandle<TReportSearchResponse> GetDefaultOfferAsync(
        TModelId modelId,
        const TCgiGlFilters& glFilters,
        const EMarketType marketType,
        const NSc::TValue& price = NSc::TValue(),
        TMaybe<ui64> hidMaybe = Nothing(),
        TMaybe<ui64> feshMaybe = Nothing());
    TResponseHandle<TReportSearchResponse> GetOfferAsync(TStringBuf wareId);
    TResponseHandle<TReportSearchResponse> GetProductOffersAsync(
        ui64 modelId,
        const NBASS::NMarket::TCgiGlFilters& glFilters,
        const NSc::TValue& price = NSc::TValue(),
        TMaybe<ui64> hidMaybe = Nothing());
    TResponseHandle<TReportSearchResponse> GetModelInfoAsync(
        ui64 modelId,
        const NBASS::NMarket::TCgiGlFilters& glFilters,
        const TRedirectCgiParams& redirectCgiParams,
        const NSc::TValue& price = NSc::TValue(),
        TMaybe<ui64> hidMaybe = Nothing());
    THashMap<i32, TString> GetAllDeliveryStatuses(const THashSet<i32>& deliveryIds);

private:
    i64 RegionId;
    i32 Clid;
    TStringBuf Uuid;
    TStringBuf Reqid;
    TStringBuf Ip;

    struct TBundleUrlInfo {
        TString BaseUrl;
        i32 Clid;

        TBundleUrlInfo(const TString& baseUrl, i32 clid)
            : BaseUrl(baseUrl)
            , Clid(clid)
        {
        }
    };
    TMaybe<TBundleUrlInfo> BundleUrlInfo;

    TCgiParameters CreateReportCgiParams();
    THashMap<TString, TString> CreateHeaders() const;
    TSourceRequestFactory GetSource(const EMarketType& marketType);
};

} // namespace NMarket

} // namespace NBASS
