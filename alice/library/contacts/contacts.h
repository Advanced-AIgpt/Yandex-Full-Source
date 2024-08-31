#pragma once

#include <alice/megamind/protos/speechkit/request.pb.h>
#include <alice/protos/data/contacts.pb.h>

#include <util/generic/string.h>

namespace NAlice::NContacts {

constexpr TStringBuf CONTACTS_CLIENT_ENTITY_NAME = "contacts";
constexpr int CONTACTS_ADDRESS_BOOK_MAX_SIZE = 2000;
constexpr size_t CONTACTS_REQUEST_TOKENS_MAX_SIZE = 20;

bool IsContactFromMessenger(const NAlice::NData::TContactsList::TContact& contact);

bool IsContactFromMessenger(const TString& accountType);

TString GetContactUniqueKey(const NAlice::NData::TContactsList::TContact& contact);

TString GetContactMatchingInfo(const NAlice::NData::TContactsList::TContact& contact);

TString SerializeLookupKeyMap(const THashMap<TString, size_t>& lookupKeyMap);

THashMap<size_t, TString> DeSerializeLookupKeyMap(const TString& serialied);

} // namespace NAlice::NContacts
