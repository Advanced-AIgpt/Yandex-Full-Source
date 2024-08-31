namespace NBassApi;

struct TFormalizedGlFilter {
    type: string (required, allowed = ["boolean", "number", "enum"]);
    values: [struct {
        id: ui64 (required);
        num: double (required);
    }];
};

struct TFormalizedGlFilters {
    filters: { ui64 -> TFormalizedGlFilter } (required);
};

struct TDeliveryInfo {
    results: [struct {
        texts: struct {
            mobile: string;
            desktop: string;
        };
        id: i32;
    }];
};
