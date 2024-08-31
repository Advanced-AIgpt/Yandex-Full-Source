#pragma once

#include <alice/hollywood/library/scenarios/market/common/proto/types.pb.h>

#include <alice/library/geo/geodb.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/json/json_value.h>
#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/string/cast.h>

namespace NAlice::NHollywood::NMarket {

using NGeobase::TId;
using TPrice = double; // TODO(bas1330) consider using fixed-point number
using TModelId = ui64;
using THid = ui64;
using TNid = ui64;

class TCategory {
public:
    TCategory(const NProto::TCategory& proto);
    TCategory(TNid nid, TStringBuf slug) : Nid(nid), Slug(TString(slug)) {}
    TCategory(THid hid) : Hid(hid) {}
    TCategory(THid hid, TNid nid, TStringBuf slug)
        : Hid(hid)
        , Nid(nid)
        , Slug(ToString(slug))
    {
    }

    bool HasHid() const { return Hid.Defined(); }
    THid GetHid() const { return Hid.GetRef(); }
    bool HasNid() const { return Nid.Defined(); }
    TNid GetNid() const { return Nid.GetRef(); }
    TStringBuf GetSlug() const { return Slug; }

    NProto::TCategory ToProto() const;

private:
    TMaybe<THid> Hid;
    TMaybe<TNid> Nid;
    TString Slug;
};

class TCgiGlFilters : public THashMap<TString, TVector<TString>> {
    using TBase = THashMap<TString, TVector<TString>>;
public:
    using TBase::TBase;
    TCgiGlFilters(const NProto::TCgiGlFilters& proto);

    void AddToCgi(TCgiParameters& cgi) const;
    NProto::TCgiGlFilters ToProto() const;
    static TCgiGlFilters FromReportFilters(const NJson::TJsonValue::TArray& reportFilters);
};

class TCgiRedirectParameters : public TCgiParameters {
public:
    using TCgiParameters::TCgiParameters;

    void AddToCgi(TCgiParameters& cgi) const;
};

enum class EMarketGoodState {
    NEW        /* "new" */,
    CUTPRICE   /* "cutprice" */,
};

enum class EClid {
    HOW_MUCH = 888,
    CHOICE_GREEN = 850,
    PRODUCT_DETAILS = 851,
    SEARCH_BY_PICTURE_IN_ALICE = 852,
    SEARCH_BY_PICTURE = 853,
    CHOICE_BLUE = 950,
    RECURRING_PURCHASE = 951,
    ORDERS_STATUS = 952,
    BERU_BONUSES = 953,
    SHOPPING_LIST = 954,
    OTHER = 0,
};
using TClidValue = ui32;

} // namespace NAlice::NHollywood::NMarket
