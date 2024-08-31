namespace NBASSTanker;

struct TTankerResponse {
    struct TRange {
        min: double;
        max: double;
    };
    struct TStation {
        id: string;
        name: string;
        city: string;
        regionId: i32;
        address: string;

        struct TFuel {
            id: string (required);
            name: string;
            marka: string;
        };
        fuels: [TFuel] (required);

        struct TLocation {
            lon : double (required);
            lat : double (required);
        };
        
        struct TColumn {
            fuels: [string];
            point: TLocation;
        };
        columns: { i32 -> TColumn } (required);

    };
    orderRange: struct {
        money: TRange;
        litre: TRange;
    };
    station: TStation;
    userFuelId: string;
    payment: struct {
        id: string;
    };
};
