#pragma once

#include "base_client.h"
#include "bool_scheme_traits.h"
#include <alice/bass/forms/market/types.h>

#include <alice/bass/forms/market/client/stock_storage.sc.h>

namespace NBASS {

namespace NMarket {

class TStockStorageClient : public TBaseClient {
public:
    explicit TStockStorageClient(TSourcesRequestFactory sources, TMarketContext& context)
        : TBaseClient(sources, context)
    {
    }

    ui64 SkuItemsNumber(TStringBuf sku, ui64 vendorId, ui64 warehouseId);
};

} // namespace NMarket

} // namespace NBASS
