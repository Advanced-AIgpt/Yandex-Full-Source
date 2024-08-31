namespace NBassApi;

struct TStockStorageGetAvailableAmountsRequest {
    items: [struct {
        shopSku: string;
        vendorId: ui64;
        warehouseId: ui64;
    }];
};

struct TStockStorageGetAvailableAmountsResponse {
    items: [struct {
        amount: i64;
    }];
};
