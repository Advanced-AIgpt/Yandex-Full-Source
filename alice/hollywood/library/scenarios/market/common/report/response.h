#pragma once

#include <alice/hollywood/library/scenarios/market/common/types.h>
#include <alice/hollywood/library/scenarios/market/common/types/picture.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <util/generic/maybe.h>
#include <util/generic/variant.h>

namespace NAlice::NHollywood::NMarket {

class TBaseReportDocument {
public:
    // TODO(bas1330) not cool to copy json docs
    TBaseReportDocument(const NJson::TJsonValue& data)
        : Data(data)
    {}

    TStringBuf GetTitle() const { return Data["titles"]["raw"].GetString(); }
    const TVector<TCategory>& GetCategories() const;

protected:
    NJson::TJsonValue Data;

private:
    mutable TMaybe<TVector<TCategory>> Categories;
};

class TReportOffer : public TBaseReportDocument {
public:
    struct TPrices {
        // Min or Value must be set
        TMaybe<TPrice> Min;
        TMaybe<TPrice> Value;
        TMaybe<TPrice> BeforeDiscount;
        TString Currency;
    };

    TReportOffer(const NJson::TJsonValue& data)
        : TBaseReportDocument(data)
        , Prices(CreatePrices())
        , Picture(TPicture::GetMostSuitablePicture(Data["pictures"].GetArray()))
    {}

    TStringBuf GetWareId() const { return Data["wareId"].GetString(); }
    TPrices GetPrices() const { return Prices; }
    const TPicture& GetPicture() const { return Picture; }
    TStringBuf GetCpc() const { return Data["cpc"].GetString(); }

private:
    TPrices Prices;
    TPicture Picture;

    TPrices CreatePrices() const;
};

class TReportModel : public TBaseReportDocument {
public:
    struct TPrices {
        TPrice Min;
        TMaybe<TPrice> Avg;
        TMaybe<TPrice> Default;
        TMaybe<TPrice> DefaultBeforeDiscount;
        TString Currency;
    };

    TReportModel(const NJson::TJsonValue& data)
        : TBaseReportDocument(data)
        , DefaultOffer(CreateDefaultOffer())
        , Prices(CreatePrices())
        , GlFilters(TCgiGlFilters::FromReportFilters(Data["filters"].GetArray()))
        , Picture(TPicture::GetMostSuitablePicture(Data["pictures"].GetArray(), GlFilters))
    {
    }

    TModelId GetId() const { return Data["id"].GetUInteger(); }
    TStringBuf GetSlug() const { return Data["slug"].GetString(); }
    const TPicture& GetPicture() const { return Picture; }
    const TPrices& GetPrices() const { return Prices; }
    const TCgiGlFilters& GetGlFilters() const { return GlFilters; }
    const TMaybe<TReportOffer>& GetDefaultOffer() const { return DefaultOffer; }

private:
    TMaybe<TReportOffer> DefaultOffer;
    TPrices Prices;
    TCgiGlFilters GlFilters;
    TPicture Picture;

    TMaybe<TReportOffer> CreateDefaultOffer() const;
    TPrices CreatePrices() const;
};

using TReportDocument = std::variant<TReportOffer, TReportModel>;

struct TReportPrimeRedirect {
    TReportPrimeRedirect(const NJson::TJsonValue& data);

    TMaybe<TString> Text;
    TMaybe<NGeobase::TId> RegionId;
    TMaybe<TCategory> Category = Nothing();
    TCgiGlFilters GlFilters = {};
    TCgiRedirectParameters RedirectParams = {};
    bool HasRedirect = false;
};

class TReportPrimeResponse {
public:
    TReportPrimeResponse(const NJson::TJsonValue& data)
        : Data(data)
    {
    }

    ui64 GetTotal() const { return Data["search"]["total"].GetUInteger(); }
    const TVector<TReportDocument>& GetDocuments() const;
    bool IsRedirect() const { return Data.Has("redirect"); }
    TReportPrimeRedirect GetRedirect() const { return { Data["redirect"] }; };

private:
    NJson::TJsonValue Data;
    mutable TMaybe<TVector<TReportDocument>> Documents_;
};

} // namespace NAlice::NHollywood::NMarket
