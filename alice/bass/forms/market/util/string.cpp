#include "string.h"

namespace NBASS {

namespace NMarket {

namespace {

void AddAddressPart(TStringBuilder& builder, TStringBuf part)
{
    if (!builder.empty() && !part.empty()) {
        builder << ", ";
    }
    builder << part;
}

template <class TAddressScheme>
TString ToAddressString(const TAddressScheme& address)
{
    TStringBuilder result;

    AddAddressPart(result, address.Country());
    AddAddressPart(result, address.City());
    AddAddressPart(result, address.Street());
    AddAddressPart(result, address.House());
    AddAddressPart(result, address.Apartment());
    return result;
}

} // namespace

TString ToAddressString(const TAddressScheme& address)
{
    return ToAddressString<TAddressScheme>(address);
}
TString ToAddressString(const TAddressSchemeConst& address)
{
    return ToAddressString<TAddressSchemeConst>(address);
}

} // namespace NMarket

} // namespace NBASS
