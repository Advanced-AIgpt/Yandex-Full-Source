#pragma once

#include "base_client.h"

namespace NBASS {

namespace NMarket {

class TCoin {
public:
    TCoin(TStringBuf Name, TStringBuf Subname, TStringBuf Image)
        : Name(Name)
        , Subname(Subname)
        , Image(Image)
    {
    }

    TString Name;
    TString Subname;
    TString Image;

    NSc::TValue ToTValue() const;
};

class TBeruBonusesClient: public TBaseClient {
public:
    explicit TBeruBonusesClient(TSourcesRequestFactory sources, TMarketContext& ctx)
        : TBaseClient(sources, ctx)
    {
    }

    TVector<TCoin> GetAllMyBonuses(TString nowUid);
};

}

}
