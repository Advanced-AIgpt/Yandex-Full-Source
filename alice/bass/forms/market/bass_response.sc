namespace NBassApi;

struct TFormUpdate {
    name: string (required);
    slots: [any];
    resubmit: bool;
};

struct TPrice {
    value: string (required);
    currency: string;
};

struct TPicture {
    height: ui64 (required);
    width: ui64 (required);
    container_height: ui64 (required);
    container_width: ui64 (required);
    original_ratio: double (required);
    url: string (required);
};

struct TWarning {
    type: string (required);
    value: string (required);
};

// todo MALISA-240 разнести схемы нанеймспейсы
struct TOutputDelivery {
    promotion: struct {
        description: string;
    };
    courier: struct {
        date: string;
        price: struct {
            value: string;
            currency: string;
        };
    };
    has_pickup: bool;
};

struct TBeruOrderCardData {
    title: string (required);
    prices: TPrice (required);
    action (required): struct {
        form_update: TFormUpdate (required);
    };
    picture: TPicture (required);
    urls (required): struct {
        terms_of_use: string (required);
        model: string (required);
        supplier: string (required);
    };
    delivery: TOutputDelivery (required);
    free_delivery: bool;
};

struct TProductOffersCardData {
    title: string (required);
    prices (required): struct {
        avg: string (required);
        min: string (required);
        max: string (required);
        currency: string (required);
    };
    picture: TPicture (required);
    rating_icon: string;
    review_count: ui32;
    colors: [string];
    warnings: [TWarning];
    urls: struct {
        reviews: string;
        model: string;
    };
    adviser_percentage: ui8;
    reasons_to_buy: [string];

    offers: struct {
        beru: struct {
            color: string;
            urls: struct {
                terms_of_use: string;
                model: string;
                supplier: string;
                market: string;
            };
            price: TPrice;
            price_before_discount: TPrice;
            add_to_cart: struct {
                form_update: TFormUpdate;
            };
            beru_order: struct {
                form_update: TFormUpdate;
            };
            // todo unify shop struct
            delivery: TOutputDelivery;
            shop: struct {
                name: string;
                rating: double;
                rating_count: ui64;
            };
        };
        other: [struct {
            color: string;
            urls: struct {
                shop: string;
                market: string;
            };
            price: TPrice;
            price_before_discount: TPrice;
            delivery: TOutputDelivery;
            shop: struct {
                name: string;
                rating: double;
                rating_count: ui64;
            };
        }];
        total: struct {
            count: ui64;
            url: string;
        };
    };
    rating_icons: {string -> string};
};

struct TCheckoutAskPhoneTextCardData {
    is_guest: bool (required);
};
