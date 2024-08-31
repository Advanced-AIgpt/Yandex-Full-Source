namespace NBASSSavedAddress;

struct TSavedAddress {
    created : string (default = "");
    modified : string (default = "");
    last_used : string (cppname = LastUsed, default = "");

    address_id : string (required, cppname = AddressId, allowed = ["home", "work"]);
    tags : string[];
    title : string (required, != "");

    latitude : double (required);
    longitude : double (required);

    address_line : string (cppname = AddressLine);
    address_line_short : string (cppname = AddressLineShort);
};
