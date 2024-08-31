#include "contacts.h"

#include <alice/library/compression/compression.h>
#include <alice/library/contacts/proto/contacts.pb.h>

#include <library/cpp/string_utils/base64/base64.h>

#include <util/generic/algorithm.h>
#include <util/generic/hash.h>
#include <util/generic/vector.h>
#include <util/string/cast.h>

namespace NAlice::NContacts {

namespace {

const TVector<TStringBuf> MESSENGER_TYPES = {
    "com.viber.voip",
    "com.whatsapp",
    "com.whatsapp.w4b",
    "org.telegram.messenger",
    "com.skype.raider",
};

} // namespace

bool IsContactFromMessenger(const NAlice::NData::TContactsList::TContact& contact) {
    return FindPtr(MESSENGER_TYPES, contact.GetAccountType());
}

bool IsContactFromMessenger(const TString& accountType) {
    return FindPtr(MESSENGER_TYPES, accountType);
}

TString GetContactUniqueKey(const NAlice::NData::TContactsList::TContact& contact) {
    if (contact.GetLookupKey().empty()) {
        return ToString(contact.GetLookupKeyIndex());
    }
    return contact.GetLookupKey();
}

TString GetContactMatchingInfo(const NAlice::NData::TContactsList::TContact& contact) {
    return contact.GetDisplayName();
}

TString SerializeLookupKeyMap(const THashMap<TString, size_t>& lookupKeyMap) {
    TLookupKeyMap lookupKeyMapProto;
    auto& mapping = *lookupKeyMapProto.MutableMapping();
    mapping.Reserve(lookupKeyMap.size());
    for (const auto& [lookupKey, index] : lookupKeyMap) {
        auto& mappingValue = *mapping.Add();
        mappingValue.SetLookupKey(lookupKey);
        mappingValue.SetLookupIndex(index);
    }

    return Base64EncodeUrl(ZLibCompress(lookupKeyMapProto.SerializeAsString()));
}

THashMap<size_t, TString> DeSerializeLookupKeyMap(const TString& serialized) {
    TLookupKeyMap lookupKeyMapProto;
    Y_PROTOBUF_SUPPRESS_NODISCARD lookupKeyMapProto.ParseFromString(ZLibDecompress(Base64Decode(serialized)));

    THashMap<size_t, TString> reversedLookupKeyMap;
    for (const auto& mappingValue : lookupKeyMapProto.GetMapping()) {
        reversedLookupKeyMap[mappingValue.GetLookupIndex()] = mappingValue.GetLookupKey();
    }

    return reversedLookupKeyMap;
}

} // namespace NAlice::NContacts
