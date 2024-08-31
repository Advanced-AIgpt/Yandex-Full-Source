#include "stock_storage_client.h"

#include "base_client.h"
#include "bool_scheme_traits.h"
#include <alice/bass/forms/market/types.h>
#include <alice/bass/forms/market/client/stock_storage.sc.h>

namespace NBASS {

namespace NMarket {

namespace {

using TGetAvailableAmountsRequestScheme = NBassApi::TStockStorageGetAvailableAmountsRequest<TBoolSchemeTraits>;
using TGetAvailableAmountsResponseScheme = NBassApi::TStockStorageGetAvailableAmountsResponse<TBoolSchemeTraits>;

}

ui64 TStockStorageClient::SkuItemsNumber(TStringBuf shopSku, ui64 vendorId, ui64 warehouseId)
{
    NSc::TValue requestData;
    TGetAvailableAmountsRequestScheme requestDataScheme(&requestData);
    auto item = requestDataScheme.Items().Add();
    item.ShopSku() = shopSku;
    item.VendorId() = vendorId;
    item.WarehouseId() = warehouseId;
    const auto& response = Run<TSchemeHolder<TGetAvailableAmountsResponseScheme>>(
        Sources.MarketStockStorage("/order/getAvailableAmounts"),
        TCgiParameters(),
        requestData
    ).Wait();

    const auto& responseData = response.GetResponse();
    const auto& resultItems = responseData->Items();
    if (resultItems.Empty()) {
        LOG(INFO) << "Have no item on the warehouse: " << requestData << Endl;
        return 0;
    }
    const auto& amount = resultItems[0].Amount();
    if (amount < 0) {
        LOG(INFO)
            << "Got negative amount (item is not available): "
            << responseData.Value() << Endl;
        return 0;
    }
    return amount;
}

} // namespace NMarket

} // namespace NBASS

