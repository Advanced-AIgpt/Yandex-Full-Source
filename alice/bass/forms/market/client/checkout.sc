namespace NBassApi;

struct TGeoResponseItem {
    address_line(required): string;
    country: string;
    city: string;
    street: string;
    house: string;
    geoid(required): ui64;
};
struct TAddress {
    country : string;
    city: string;
    street: string;
    house: string;
    apartment: string;
    recipient: string;
    phone: string;
    text: string;
    regionId: ui64;
};
struct TBuyer {
    lastName: string;
    firstName: string;
    middleName: string;
    phone: string;
    email: string;
    dontCall: bool;
};
struct TCartDeliveryOption {
    id: string;
    outlets: [struct {
        id: i64;
    }];
    deliveryOptionId: string;
    type: string;
    buyerPrice: ui64;
    dates: struct {
        fromDate: string;
        toDate: string;
    };
    deliveryIntervals: [struct {
        date: string;
        intervals: [struct {
            fromTime: string;
            toTime: string;
            isDefault: bool;
        }];
    }];
    paymentOptions: [struct {
        paymentType: string;
        paymentMethod: string;
    }];
};
struct TDeliveryDates {
    buyerPrice: ui64;
    fromDate: string;
    fromTime: string;
    isDefault: bool;
    optionId: string;
    toDate: string;
    toTime: string;
};
struct TCartDelivery {
    id: string;
    regionId: i64;
    outletId: i64;
    buyerAddress: TAddress;
    dates: TDeliveryDates;
};
struct TOrderItem {
    feedId: ui64;
    offerId: string;
    buyerPrice: double;
    showInfo: string;
    count: i32;
    sku: string;
    modelId: ui64;
};
struct TCart {
    shopId: ui64;
    delivery: TCartDelivery;
    items: [TOrderItem];
    deliveryOptions: [TCartDeliveryOption];
    notes: string;
};
struct TCheckouterData {
    buyerRegionId: i64;
    buyerCurrency: string;
    carts: [TCart];
    buyer: TBuyer;
};
struct TCheckoutRequest {
    buyerRegionId: i64;
    buyerCurrency: string;
    orders: [TCart];
    paymentMethod: string;
    paymentType: string;
    buyer: TBuyer;
};
// todo: MALISA-240 вынести TReportDefaultOfferBlue из этого файла
struct TReportFilter {
    xslname (cppname = XslName): string;
    subType: string;
    values: [struct {
        code: string;
    }];
};
struct TReportDocument {
    entity: string;
    id: ui32;
    vendor: struct {
        id: ui32;
    };
    titles: struct {
        raw: string;
    };
    categories: [struct {
        id: ui32;
    }];
    model: struct {
        id: ui32;
    };
    shop: struct {
        id: ui32;
        name: string;
        feed: struct {
            id: string;
            offerId: string;
        };
        qualityRating: double;
        overallGradesCount: ui64;
    };
    prices: struct {
        currency: string;
        discount: struct {
            oldMin: string;
        };
        value: string;
        min: string;
        max: string;
        avg: string;
    };
    marketSku: string;
    wareId: string;
    cpc: string;
    slug: string;
    feeShow: string;
    pictures: [struct {
        original: struct {
            url: string;
            height: ui64;
            width: ui64;
        };
        thumbnails: [struct {
            url: string;
            height: ui64;
            width: ui64;
        }];
    }];
    delivery: struct {
        options: [struct {
            price: struct {
                currency: string;
                value: string;
            };
            dayFrom: ui32;
        }];
        hasPickup: bool;
    };
    warnings: struct {
        common: [struct {
            type: string;
            value: struct {
                full: string;
                short: string;
            };
        }];
    };
    supplier: struct {
        id: ui32;
        warehouseId: ui32;
    };
    stockStoreCount: ui32;
    supplierSku: string;
    urls: struct {
        cpa: string;
        encrypted: string;
    };
    filters: [TReportFilter];
    rating: double;
};
struct TReportDefaultOfferBlue {
    search (required): struct {
        results (required): [TReportDocument];
    };
};
// todo MALISA-240: повторяется много кода с другими респонсами, нужно их унифицировать
struct TReportSku {
    search (required): struct {
        results (required): [struct {
            product (required): struct {
                id (required): ui64;
            };
            offers (required): struct {
                items: [TReportDocument];
            };
            specs: struct {
                friendly: [string];
            };
        }];
    };
};
struct TReportSearchResponse {
    search: struct {
        total: ui64;
        totalOffers: ui64;
        results: [TReportDocument];
    };
    filters: [TReportFilter];
};
struct TDelivery {
    type: string;
    address: TAddress;

    // specific to type="PICKUP"
    outlet: struct {
        id: i64;
        name: string;
    };
};
struct TDeliveryOption {
    deliveryId: string;
    price: ui64;
    dates: struct {
        fromDate: string;
        toDate: string;

        // specific to type="DELIVERY"
        fromTime: string;
        toTime: string;
    };

    // specific to type="DELIVERY"
    optionId: string;
};

struct TStateCart {
    offer: TReportDocument;
    itemsNumber: ui32;
};

struct TMarketCheckoutState {
    step: string;
    attempt: ui32 (default = 0);
    attemptReminder : bool (default = false);
    muid: string;
    email: string;
    phone: string;
    buyerAddress: TAddress;
    cart: TStateCart;
    delivery: TDelivery;
    deliveryOption: TDeliveryOption;

    // for state "address"
    deliverySuggests: [struct {
        delivery: TDelivery;
        prices: struct {
            min: ui64;
            max: ui64;
        };
    }];

    // for state "deliveryOptions"
    deliveryOptions: [TDeliveryOption];

    sku: ui64;
    order: struct {
        checkouted_at_timestamp: ui64 (required);
        alice_id: string (required);
        attempt: ui32 (required);
        id: ui64;
    };
};
