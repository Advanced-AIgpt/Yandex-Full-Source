namespace NBassApi;

struct TTimeInterval {
    from: string;
    to: string;
};

struct TSloboda {
    facts: [struct {
        key_words: [string];
        texts: [string];
    }];
};

struct TPromotions {
    free_delivery_interval: TTimeInterval;

    free_delivery_by_vendor: {
        ui32 -> struct {
            interval: TTimeInterval;
            description: string;
        }
    };

    sloboda: TSloboda;
};
