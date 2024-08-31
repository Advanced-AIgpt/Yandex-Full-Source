#include "saved_address.h"

#include <alice/bass/forms/context/context.h>
#include <alice/bass/forms/geo_resolver.h>
#include <alice/bass/util/error.h>

#include <alice/bass/libs/logging_v2/logger.h>

#include <util/datetime/base.h>

namespace NBASS {

TSavedAddress& TSavedAddress::operator=(const TSavedAddress& other) {
    if (this == &other) {
        return *this;
    }

    AddressRaw = other.AddressRaw.Clone();
    Address.reset(new TSavedAddressConstScheme(&AddressRaw));
    return *this;
}

TResultValue TSavedAddress::FromGeoPos(const TStringBuf& addressId, const TGeoPosition& pos, const ui64 epoch) {
    if (addressId.empty()) {
        return TError(TError::EType::SYSTEM, "can not create saved address with empty address_id");
    }

    AddressRaw.Clear();
    TString creationTime = TInstant::Seconds(epoch).ToString();

    AddressRaw["created"] = creationTime;
    AddressRaw["modified"] = creationTime;
    AddressRaw["last_used"] = creationTime;

    AddressRaw["latitude"] = pos.Lat;
    AddressRaw["longitude"] = pos.Lon;

    AddressRaw["address_id"].SetString(addressId);
    AddressRaw["tags"].SetArray().Push().SetString(addressId);
    AddressRaw["title"].SetString(addressId);

    Address.reset(new TSavedAddressConstScheme(&AddressRaw));

    if (!Address->Validate()) {
        return TError(TError::EType::SYSTEM, "can not create valid saved address structure from geo");
    }

    return TResultValue();
}

TResultValue TSavedAddress::FromGeo(const TStringBuf& addressId, const NSc::TValue& geoAddress, const ui64 epoch) {
    const NSc::TValue& location = geoAddress["location"];
    TGeoPosition geoPos(location["lat"].GetNumber(), location["lon"].GetNumber());

    if (TResultValue err = FromGeoPos(addressId, geoPos, epoch)) {
        LOG(DEBUG) << "Invalid saved address : " << AddressRaw.ToJson() << "; was created from geo: " << geoAddress.ToJson();
        return err;
    }

    return TResultValue();
}

bool TSavedAddress::IsNull() const {
    return !Address || Address->IsNull();
}

bool TSavedAddress::IsValid() const {
    return !IsNull() && Address->Validate();
}

NSc::TValue TSavedAddress::Value() const {
    if (IsNull()) {
        return NSc::TValue();
    }

    return *(Address->GetRawValue());
}

TString TSavedAddress::ToJson() const {
    return Value().ToJson();
}

double TSavedAddress::Latitude() const {
    if (IsNull()) {
        return 0;
    }
    return Address->Latitude();
}

double TSavedAddress::Longitude() const {
    if (IsNull()) {
        return 0;
    }
    return Address->Longitude();
}

}
