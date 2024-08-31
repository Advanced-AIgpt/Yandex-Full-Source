#include <alice/library/contacts/contacts.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/random/shuffle.h>

using namespace NAlice::NContacts;

Y_UNIT_TEST_SUITE(Contacts) {
    Y_UNIT_TEST(SerializeDeserialize) {
        const TString key1 = "some_test_1";
        const TString key2 = "some_test_2";

        THashMap<TString, size_t> lookupKeyMap;
        lookupKeyMap[key1] = 1;
        lookupKeyMap[key2] = 2;

        const auto& serialized = SerializeLookupKeyMap(lookupKeyMap);

        THashMap<size_t, TString> reversedLookupKeyMap = DeSerializeLookupKeyMap(serialized);
        UNIT_ASSERT_EQUAL(lookupKeyMap.size(), reversedLookupKeyMap.size());
        UNIT_ASSERT_EQUAL(reversedLookupKeyMap[1], key1);
        UNIT_ASSERT_EQUAL(reversedLookupKeyMap[2], key2);
    }

    Y_UNIT_TEST(CheckSize) {
        const size_t numContacts = 10;
        TString lookupKey = "3056r5210-278E06A246030827B68AC828A2628A728A788A9C0308278E968A9C8E03081B275403170A8A27C00315230819.3789r5211-278E06A246030827B68AC828A2628A728A788A9C0308278E968A9C8E03081B275403170A8A27C00315230819.248r5212-278E06A246030827B68AC828A2628A728A788A9C0308278E968A9C8E03081B275403170A8A27C00315230819.3675i5210.3841r5216-278E06A246030827B68AC828A2628A728A788A9C0308278E968A9C8E03081B275403170A8A27C00315230819.902r5217-278E06A246030827B68AC828A2628A728A788A9C0308278E968A9C8E03081B275403170A8A27C00315230819";

        THashMap<TString, size_t> lookupKeyMap;
        for (size_t i = 0; i < numContacts; ++i) {
            Shuffle(lookupKey.begin(), lookupKey.end());
            lookupKeyMap[lookupKey] = i;
        }

        const auto& serialized = SerializeLookupKeyMap(lookupKeyMap);

        THashMap<size_t, TString> reversedLookupKeyMap = DeSerializeLookupKeyMap(serialized);
        UNIT_ASSERT_EQUAL(lookupKeyMap.size(), reversedLookupKeyMap.size());

        const size_t serializedBytes = serialized.size() + lookupKeyMap.size() * 4;
        const size_t defaultBytes = 2 * lookupKey.size() * numContacts;
        UNIT_ASSERT(serializedBytes < defaultBytes);
    }
}
