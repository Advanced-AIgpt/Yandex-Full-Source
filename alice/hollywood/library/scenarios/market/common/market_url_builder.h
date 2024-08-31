#pragma once

#include "types.h"
#include <alice/hollywood/library/request/request.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NAlice::NHollywood::NMarket {

class TMarketUrlBuilder {
public:
    enum EMarketModelTab {
        Main,
        Offers,
        Reviews,
    };

    TMarketUrlBuilder(
        EClid clid,
        TStringBuf uuid,
        TStringBuf reqid,
        bool isTouch);
    TMarketUrlBuilder(const TMarketUrlBuilder&) = default;

    TString GetMarketSearchUrl(
        TStringBuf text,
        TMaybe<NGeobase::TId> regionId = Nothing(),
        TMaybe<EMarketGoodState> goodState = Nothing(),
        bool redirect = false) const;
    TString GetMarketCategoryUrl(
        const TCategory& category,
        const TCgiGlFilters& glFilters = {},
        TMaybe<NGeobase::TId> regionId = Nothing(),
        TMaybe<EMarketGoodState> goodState = Nothing(),
        TStringBuf text = "",
        const TCgiRedirectParameters& redirectParams = {}) const;
    TString GetMarketModelUrl(
        TModelId,
        TStringBuf slug,
        TMaybe<NGeobase::TId> regionId = Nothing(),
        const TCgiGlFilters& glFilters = {},
        const TCgiRedirectParameters& redirectParams = {},
        EMarketModelTab tab = EMarketModelTab::Main) const;
    TString GetMarketOfferUrl(
        TStringBuf ware_md5,
        TStringBuf cpc,
        TMaybe<NGeobase::TId> regionId = Nothing()) const;

private:
    EClid Clid;
    TString Uuid;
    TString Reqid;
    bool IsTouch;

    TStringBuf GetBaseUrl() const;
    TCgiParameters GetBaseCgi(TMaybe<NGeobase::TId> regionId) const;
    static TString GetSlugWithPrefix(TStringBuf slug);
    static TStringBuf GetModelTabPath(EMarketModelTab tab);
    TString GetMarketUrl(TStringBuf uri = TStringBuf("")) const;
};

TMarketUrlBuilder CreateMarketUrlBuilder(EClid clid, const TScenarioBaseRequestWrapper& request);

} // namespace NAlice::NHollywood::NMarket
