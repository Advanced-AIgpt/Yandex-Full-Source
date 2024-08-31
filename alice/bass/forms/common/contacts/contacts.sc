namespace NBassApi;

// full name as sent from VINS
struct TFio {
    surname             : string;
    name                : string;
    patronym            : string;
};

// Contact as sent from device: one record per name and phone
struct TRawContact {
    name                : string (required);
    phone               : string (required);
    phone_type_id       : i32;
    phone_type_name     : string;
    times_contacted     : i32;
    last_time_contacted : i64;  // timestamp in milliseconds
    account_type        : string;
};

struct TPhone {
    phone               : string (required);
    phone_type_id       : i32;
    phone_type_name     : string;
    times_contacted     : i32;
    last_time_contacted : i64;
    account_type        : string;
};

// Processed contact with a list of phones
struct TContact {
    name                : string (required);
    phones              : [ TPhone ];
};

struct TContactSearchQuery {
    values              : [ string ] (required);  // values to look up from SQLite storage
    condition           : string;  // a part of raw SQL query. remove when client starts supporting 'values'
};
