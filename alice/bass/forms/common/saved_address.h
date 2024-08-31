#pragma once

#include <alice/library/geo_resolver/geo_position.h>

#include <alice/bass/util/error.h>

#include <alice/bass/forms/common/saved_address.sc.h>

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/scheme/domscheme_traits.h>

namespace {

using TSavedAddressConstScheme = NBASSSavedAddress::TSavedAddressConst<TSchemeTraits>;

} // end anon namespace

namespace NBASS {

class TContext;

using TGeoPosition = NAlice::TGeoPosition;

class TSavedAddress {
public:

    TSavedAddress(const NSc::TValue& address)
        : AddressRaw(address)
    {
        Address = std::make_unique<TSavedAddressConstScheme>(&AddressRaw);
    }

    TSavedAddress()
    {}

    TSavedAddress(const TSavedAddress& other)
        : TSavedAddress(other.AddressRaw)
    {}

    TSavedAddress& operator=(const TSavedAddress& other);

    TResultValue FromGeoPos(const TStringBuf& addressId, const TGeoPosition& pos, const ui64 epoch);
    TResultValue FromGeo(const TStringBuf& addressId, const NSc::TValue& geoAddress, const ui64 epoch);

    bool IsNull() const;
    bool IsValid() const;
    NSc::TValue Value() const;
    TString ToJson() const;

    double Latitude() const;
    double Longitude() const;

private:
    NSc::TValue AddressRaw;
    std::unique_ptr<TSavedAddressConstScheme> Address;
};

}
