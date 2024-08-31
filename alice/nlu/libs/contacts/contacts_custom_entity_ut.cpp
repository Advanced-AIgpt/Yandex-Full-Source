#include "contacts_custom_entity.h"

#include <alice/library/proto/proto.h>
#include <alice/nlu/libs/entity_searcher/entity_searcher.h>
#include <alice/nlu/libs/entity_searcher/entity_searcher_builder.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/random/shuffle.h>

using namespace NAlice::NContacts;
using namespace NAlice::NNlu;

Y_UNIT_TEST_SUITE(NCustomEntity_ParseContacts) {

     NAlice::TClientEntity GenerateRandomContacts(const TString& sourceDisplayName, size_t numCountacts) {
        TString lookupKey = "3056r5210-278E06A246030827B68AC828A2628A728A788A9C0308278E968A9C8E03081B275403170A8A27C00315230819.3789r5211-278E06A246030827B68AC828A2628A728A788A9C0308278E968A9C8E03081B275403170A8A27C00315230819.248r5212-278E06A246030827B68AC828A2628A728A788A9C0308278E968A9C8E03081B275403170A8A27C00315230819.3675i5210.3841r5216-278E06A246030827B68AC828A2628A728A788A9C0308278E968A9C8E03081B275403170A8A27C00315230819.902r5217-278E06A246030827B68AC828A2628A728A788A9C0308278E968A9C8E03081B275403170A8A27C00315230819";
        TString displayName = sourceDisplayName;

        NAlice::TClientEntity contactsEntity;
        contactsEntity.SetName("contacts");

        for (size_t i = 0; i < numCountacts; ++i) {
            Shuffle(lookupKey.begin(), lookupKey.end());
            Shuffle(displayName.begin(), displayName.end());
            NAlice::TNluHint nluHint;
            auto& instance = *nluHint.AddInstances();
            instance.SetPhrase(displayName);
            (*contactsEntity.MutableItems())[lookupKey] = std::move(nluHint);
        }

        return contactsEntity;
    }

    Y_UNIT_TEST(ParseContacts_Empty) {
        {
            const TStringBuf contactsProtoText = "";
            NAlice::TClientEntity contactsProto = NAlice::ParseProtoText<NAlice::TClientEntity>(contactsProtoText);
            const TVector<TEntityString> contacts = ParseContacts(contactsProto, ELanguage::LANG_RUS);
            UNIT_ASSERT(contacts.empty());
        }
        {
            const TStringBuf contactsProtoText = R"(
                Items {
                    key: "key",
                    value: {
                        Instances: [
                            {Phrase: ":,?!&$%^-=+"}
                        ]
                    }
                }
            )";
            NAlice::TClientEntity contactsProto = NAlice::ParseProtoText<NAlice::TClientEntity>(contactsProtoText);
            const TVector<TEntityString> contacts = ParseContacts(contactsProto, ELanguage::LANG_RUS);
            UNIT_ASSERT(contacts.empty());
        }
    }

    Y_UNIT_TEST(ParseContacts_SomeContacts) {
        {
            const TStringBuf contactsProtoText = R"(
                Items {
                    key: "vitaly",
                    value: {
                        Instances: [
                            {Phrase: "Витя"}
                        ]
                    }
                }
            )";

            NAlice::TClientEntity contactsProto = NAlice::ParseProtoText<NAlice::TClientEntity>(contactsProtoText);
            const TVector<TEntityString> contacts = ParseContacts(contactsProto, ELanguage::LANG_RUS);
            const TVector<TEntityString> expectedContacts = {
                { "витя", "device.address_book.item_name", "vitaly" }
            };
            UNIT_ASSERT_VALUES_EQUAL(expectedContacts, contacts);
            TEntitySearcher entitySearcher(TEntitySearcherDataBuilder().Build(contacts));
            Y_UNUSED(entitySearcher);
        }
        {
            const TStringBuf contactsProtoText = R"(
                Items {
                    key: "vitaly",
                    value: {
                        Instances: [
                            {Phrase: "на d в 2"}
                        ]
                    }
                }
            )";

            NAlice::TClientEntity contactsProto = NAlice::ParseProtoText<NAlice::TClientEntity>(contactsProtoText);
            TVector<TEntityString> contacts = ParseContacts(contactsProto, ELanguage::LANG_RUS);
            Sort(contacts);
            const TVector<TEntityString> expectedContacts = {
                {"2", "device.address_book.item_name", "vitaly", -15},
                {"2 d", "device.address_book.item_name", "vitaly", -10},
                {"2 d в", "device.address_book.item_name", "vitaly", -5},
                {"2 d в на", "device.address_book.item_name", "vitaly", 0},
                {"2 d на", "device.address_book.item_name", "vitaly", -5},
                {"2 d на в", "device.address_book.item_name", "vitaly", 0},
                {"2 в", "device.address_book.item_name", "vitaly", -10},
                {"2 в d", "device.address_book.item_name", "vitaly", -5},
                {"2 в d на", "device.address_book.item_name", "vitaly", 0},
                {"2 в на", "device.address_book.item_name", "vitaly", -5},
                {"2 в на d", "device.address_book.item_name", "vitaly", 0},
                {"2 на", "device.address_book.item_name", "vitaly", -10},
                {"2 на d", "device.address_book.item_name", "vitaly", -5},
                {"2 на d в", "device.address_book.item_name", "vitaly", 0},
                {"2 на в", "device.address_book.item_name", "vitaly", -5},
                {"2 на в d", "device.address_book.item_name", "vitaly", 0},
                {"d 2", "device.address_book.item_name", "vitaly", -10},
                {"d 2 в", "device.address_book.item_name", "vitaly", -5},
                {"d 2 в на", "device.address_book.item_name", "vitaly", 0},
                {"d 2 на", "device.address_book.item_name", "vitaly", -5},
                {"d 2 на в", "device.address_book.item_name", "vitaly", 0},
                {"d в", "device.address_book.item_name", "vitaly", -10},
                {"d в 2", "device.address_book.item_name", "vitaly", -5},
                {"d в 2 на", "device.address_book.item_name", "vitaly", 0},
                {"d в на", "device.address_book.item_name", "vitaly", -5},
                {"d в на 2", "device.address_book.item_name", "vitaly", 0},
                {"d на", "device.address_book.item_name", "vitaly", -10},
                {"d на 2", "device.address_book.item_name", "vitaly", -5},
                {"d на 2 в", "device.address_book.item_name", "vitaly", 0},
                {"d на в", "device.address_book.item_name", "vitaly", -5},
                {"d на в 2", "device.address_book.item_name", "vitaly", 0},
                {"в 2", "device.address_book.item_name", "vitaly", -10},
                {"в 2 d", "device.address_book.item_name", "vitaly", -5},
                {"в 2 d на", "device.address_book.item_name", "vitaly", 0},
                {"в 2 на", "device.address_book.item_name", "vitaly", -5},
                {"в 2 на d", "device.address_book.item_name", "vitaly", 0},
                {"в d", "device.address_book.item_name", "vitaly", -10},
                {"в d 2", "device.address_book.item_name", "vitaly", -5},
                {"в d 2 на", "device.address_book.item_name", "vitaly", 0},
                {"в d на", "device.address_book.item_name", "vitaly", -5},
                {"в d на 2", "device.address_book.item_name", "vitaly", 0},
                {"в на", "device.address_book.item_name", "vitaly", -10},
                {"в на 2", "device.address_book.item_name", "vitaly", -5},
                {"в на 2 d", "device.address_book.item_name", "vitaly", 0},
                {"в на d", "device.address_book.item_name", "vitaly", -5},
                {"в на d 2", "device.address_book.item_name", "vitaly", 0},
                {"на 2", "device.address_book.item_name", "vitaly", -10},
                {"на 2 d", "device.address_book.item_name", "vitaly", -5},
                {"на 2 d в", "device.address_book.item_name", "vitaly", 0},
                {"на 2 в", "device.address_book.item_name", "vitaly", -5},
                {"на 2 в d", "device.address_book.item_name", "vitaly", 0},
                {"на d", "device.address_book.item_name", "vitaly", -10},
                {"на d 2", "device.address_book.item_name", "vitaly", -5},
                {"на d 2 в", "device.address_book.item_name", "vitaly", 0},
                {"на d в", "device.address_book.item_name", "vitaly", -5},
                {"на d в 2", "device.address_book.item_name", "vitaly", 0},
                {"на в", "device.address_book.item_name", "vitaly", -10},
                {"на в 2", "device.address_book.item_name", "vitaly", -5},
                {"на в 2 d", "device.address_book.item_name", "vitaly", 0},
                {"на в d", "device.address_book.item_name", "vitaly", -5},
                {"на в d 2", "device.address_book.item_name", "vitaly", 0}
            };
            UNIT_ASSERT_VALUES_EQUAL(expectedContacts, contacts);
            TEntitySearcher entitySearcher(TEntitySearcherDataBuilder().Build(contacts));
            Y_UNUSED(entitySearcher);
        }
        {
            const TStringBuf contactsProtoText = R"(
                Items {
                    key: "vitaly",
                    value: {
                        Instances: [
                            {Phrase: "Витя"}
                        ]
                    }
                }
                Items {
                    key: "petya",
                    value: {
                        Instances: [
                            {Phrase: "Petya"}
                        ]
                    }
                }
            )";

            NAlice::TClientEntity contactsProto = NAlice::ParseProtoText<NAlice::TClientEntity>(contactsProtoText);
            TVector<TEntityString> contacts = ParseContacts(contactsProto, ELanguage::LANG_RUS);
            Sort(contacts);
            const TVector<TEntityString> expectedContacts = {
                { "petya", "device.address_book.item_name", "petya", 0.0 },
                { "витя", "device.address_book.item_name", "vitaly", 0.0 }
            };
            UNIT_ASSERT_VALUES_EQUAL(expectedContacts, contacts);
            TEntitySearcher entitySearcher(TEntitySearcherDataBuilder().Build(contacts));
            Y_UNUSED(entitySearcher);
        }
        {
            const TStringBuf contactsProtoText = R"(
                Items {
                    key: "vitaly",
                    value: {
                        Instances: [
                            {Phrase: "Витя = Пётрушка Васильевич!!!!"}
                        ]
                    }
                }
            )";

            NAlice::TClientEntity contactsProto = NAlice::ParseProtoText<NAlice::TClientEntity>(contactsProtoText);
            TVector<TEntityString> contacts = ParseContacts(contactsProto, ELanguage::LANG_RUS);
            Sort(contacts);
            const TVector<TEntityString> expectedContacts = {
                { "васильевич", "device.address_book.item_name", "vitaly", -10.0 },
                { "васильевич витя", "device.address_book.item_name", "vitaly", -5.0 },
                { "васильевич витя петрушка", "device.address_book.item_name", "vitaly", 0.0 },
                { "васильевич петрушка", "device.address_book.item_name", "vitaly", -5.0 },
                { "васильевич петрушка витя", "device.address_book.item_name", "vitaly", 0.0 },
                { "витя", "device.address_book.item_name", "vitaly", -10.0 },
                { "витя васильевич", "device.address_book.item_name", "vitaly", -5.0 },
                { "витя васильевич петрушка", "device.address_book.item_name", "vitaly", 0.0 },
                { "витя петрушка", "device.address_book.item_name", "vitaly", -5.0 },
                { "витя петрушка васильевич", "device.address_book.item_name", "vitaly", 0.0 },
                { "петрушка", "device.address_book.item_name", "vitaly", -10.0 },
                { "петрушка васильевич", "device.address_book.item_name", "vitaly", -5.0 },
                { "петрушка васильевич витя", "device.address_book.item_name", "vitaly", 0.0 },
                { "петрушка витя", "device.address_book.item_name", "vitaly", -5.0 },
                { "петрушка витя васильевич", "device.address_book.item_name", "vitaly", 0.0 },
            };
            UNIT_ASSERT_VALUES_EQUAL(expectedContacts, contacts);
            TEntitySearcher entitySearcher(TEntitySearcherDataBuilder().Build(contacts));
            Y_UNUSED(entitySearcher);
        }
        {
            const TStringBuf contactsProtoText = R"(
                Items {
                    key: "vitaly",
                    value: {
                        Instances: [
                            {Phrase: "Витя = Петрушка Васильевич!!!!2"}
                        ]
                    }
                }
            )";

            NAlice::TClientEntity contactsProto = NAlice::ParseProtoText<NAlice::TClientEntity>(contactsProtoText);
            TVector<TEntityString> contacts = ParseContacts(contactsProto, ELanguage::LANG_RUS);
            Sort(contacts);
            const TVector<TEntityString> expectedContacts = {
                {"2", "device.address_book.item_name", "vitaly", -15.0 },
                {"2 васильевич", "device.address_book.item_name", "vitaly", -10.0 },
                {"2 васильевич витя", "device.address_book.item_name", "vitaly", -5.0 },
                {"2 васильевич витя петрушка", "device.address_book.item_name", "vitaly", 0.0 },
                {"2 васильевич петрушка", "device.address_book.item_name", "vitaly", -5.0 },
                {"2 васильевич петрушка витя", "device.address_book.item_name", "vitaly", 0.0 },
                {"2 витя", "device.address_book.item_name", "vitaly", -10.0 },
                {"2 витя васильевич", "device.address_book.item_name", "vitaly", -5.0 },
                {"2 витя васильевич петрушка", "device.address_book.item_name", "vitaly", 0.0 },
                {"2 витя петрушка", "device.address_book.item_name", "vitaly", -5.0 },
                {"2 витя петрушка васильевич", "device.address_book.item_name", "vitaly", 0.0},
                {"2 петрушка", "device.address_book.item_name", "vitaly", -10.0 },
                {"2 петрушка васильевич", "device.address_book.item_name", "vitaly", -5.0 },
                {"2 петрушка васильевич витя", "device.address_book.item_name", "vitaly", 0.0 },
                {"2 петрушка витя", "device.address_book.item_name", "vitaly", -5.0 },
                {"2 петрушка витя васильевич", "device.address_book.item_name", "vitaly", 0.0 },
                {"васильевич", "device.address_book.item_name", "vitaly", -15.0 },
                {"васильевич 2", "device.address_book.item_name", "vitaly", -10.0 },
                {"васильевич 2 витя", "device.address_book.item_name", "vitaly", -5.0 },
                {"васильевич 2 витя петрушка", "device.address_book.item_name", "vitaly", 0.0 },
                {"васильевич 2 петрушка", "device.address_book.item_name", "vitaly", -5.0 },
                {"васильевич 2 петрушка витя", "device.address_book.item_name", "vitaly", 0.0 },
                {"васильевич витя", "device.address_book.item_name", "vitaly", -10.0 },
                {"васильевич витя 2", "device.address_book.item_name", "vitaly", -5.0 },
                {"васильевич витя 2 петрушка", "device.address_book.item_name", "vitaly", 0.0 },
                {"васильевич витя петрушка", "device.address_book.item_name", "vitaly", -5.0 },
                {"васильевич витя петрушка 2", "device.address_book.item_name", "vitaly", 0.0 },
                {"васильевич петрушка", "device.address_book.item_name", "vitaly", -10.0 },
                {"васильевич петрушка 2", "device.address_book.item_name", "vitaly", -5.0 },
                {"васильевич петрушка 2 витя", "device.address_book.item_name", "vitaly", 0.0 },
                {"васильевич петрушка витя", "device.address_book.item_name", "vitaly", -5.0 },
                {"васильевич петрушка витя 2", "device.address_book.item_name", "vitaly", 0.0 },
                {"витя", "device.address_book.item_name", "vitaly", -15.0 },
                {"витя 2", "device.address_book.item_name", "vitaly", -10.0 },
                {"витя 2 васильевич", "device.address_book.item_name", "vitaly", -5.0 },
                {"витя 2 васильевич петрушка", "device.address_book.item_name", "vitaly", 0.0 },
                {"витя 2 петрушка", "device.address_book.item_name", "vitaly", -5.0 },
                {"витя 2 петрушка васильевич", "device.address_book.item_name", "vitaly", 0.0 },
                {"витя васильевич", "device.address_book.item_name", "vitaly", -10.0 },
                {"витя васильевич 2", "device.address_book.item_name", "vitaly", -5.0 },
                {"витя васильевич 2 петрушка", "device.address_book.item_name", "vitaly", 0.0 },
                {"витя васильевич петрушка", "device.address_book.item_name", "vitaly", -5.0 },
                {"витя васильевич петрушка 2", "device.address_book.item_name", "vitaly", 0.0 },
                {"витя петрушка", "device.address_book.item_name", "vitaly", -10.0 },
                {"витя петрушка 2", "device.address_book.item_name", "vitaly", -5.0 },
                {"витя петрушка 2 васильевич", "device.address_book.item_name", "vitaly", 0.0 },
                {"витя петрушка васильевич", "device.address_book.item_name", "vitaly", -5.0 },
                {"витя петрушка васильевич 2", "device.address_book.item_name", "vitaly", 0.0 },
                {"петрушка", "device.address_book.item_name", "vitaly", -15.0 },
                {"петрушка 2", "device.address_book.item_name", "vitaly", -10.0 },
                {"петрушка 2 васильевич", "device.address_book.item_name", "vitaly", -5.0 },
                {"петрушка 2 васильевич витя", "device.address_book.item_name", "vitaly", 0.0 },
                {"петрушка 2 витя", "device.address_book.item_name", "vitaly", -5.0 },
                {"петрушка 2 витя васильевич", "device.address_book.item_name", "vitaly", 0.0 },
                {"петрушка васильевич", "device.address_book.item_name", "vitaly", -10.0 },
                {"петрушка васильевич 2", "device.address_book.item_name", "vitaly", -5.0 },
                {"петрушка васильевич 2 витя", "device.address_book.item_name", "vitaly", 0.0 },
                {"петрушка васильевич витя", "device.address_book.item_name", "vitaly", -5.0 },
                {"петрушка васильевич витя 2", "device.address_book.item_name", "vitaly", 0.0 },
                {"петрушка витя", "device.address_book.item_name", "vitaly", -10.0 },
                {"петрушка витя 2", "device.address_book.item_name", "vitaly", -5.0 },
                {"петрушка витя 2 васильевич", "device.address_book.item_name", "vitaly", 0.0 },
                {"петрушка витя васильевич", "device.address_book.item_name", "vitaly", -5.0 },
                {"петрушка витя васильевич 2", "device.address_book.item_name", "vitaly", 0.0},
            };
            UNIT_ASSERT_VALUES_EQUAL(expectedContacts, contacts);
            TEntitySearcher entitySearcher(TEntitySearcherDataBuilder().Build(contacts));
            Y_UNUSED(entitySearcher);
        }
    }

    Y_UNIT_TEST(ParseContacts_Random) {
        const size_t eps = 50;
        {
            const TString displayName = "abcdef";
            NAlice::TClientEntity contactsProto = GenerateRandomContacts(displayName, 2000);
            const TVector<TEntityString> contacts = ParseContacts(contactsProto, ELanguage::LANG_RUS);
            UNIT_ASSERT(!contacts.empty());
            UNIT_ASSERT(contacts.size() <= MAX_CONSIDERED_VARIANTS + eps);
        }
        {
            const TString displayName = "abcdef ghijklm";
            NAlice::TClientEntity contactsProto = GenerateRandomContacts(displayName, 2000);
            const TVector<TEntityString> contacts = ParseContacts(contactsProto, ELanguage::LANG_RUS);
            UNIT_ASSERT(!contacts.empty());
            UNIT_ASSERT(contacts.size() <= MAX_CONSIDERED_VARIANTS + eps);
        }
        {
            const TString displayName = "abcdef ghijklm nopqrst";
            NAlice::TClientEntity contactsProto = GenerateRandomContacts(displayName, 2000);
            const TVector<TEntityString> contacts = ParseContacts(contactsProto, ELanguage::LANG_RUS);
            UNIT_ASSERT(!contacts.empty());
            UNIT_ASSERT(contacts.size() <= MAX_CONSIDERED_VARIANTS + eps);
        }
        {
            const TString displayName = "abcdef ghijklm nopqrst uvwxyz";
            NAlice::TClientEntity contactsProto = GenerateRandomContacts(displayName, 2000);
            const TVector<TEntityString> contacts = ParseContacts(contactsProto, ELanguage::LANG_RUS);
            UNIT_ASSERT(!contacts.empty());
            UNIT_ASSERT(contacts.size() <= MAX_CONSIDERED_VARIANTS + eps);
        }
        {
            const TString displayName = "abcdef ghijklm nopqrst uvw xyz";
            NAlice::TClientEntity contactsProto = GenerateRandomContacts(displayName, 2000);
            const TVector<TEntityString> contacts = ParseContacts(contactsProto, ELanguage::LANG_RUS);
            UNIT_ASSERT(!contacts.empty());
            UNIT_ASSERT(contacts.size() <= MAX_CONSIDERED_VARIANTS + eps);
        }
        {
            const TString displayName = "abcdef ghijklm nopqrst uvw xyz";
            NAlice::TClientEntity contactsProto = GenerateRandomContacts(displayName, 5000);
            const TVector<TEntityString> contacts = ParseContacts(contactsProto, ELanguage::LANG_RUS);
            UNIT_ASSERT(!contacts.empty());
            UNIT_ASSERT(contacts.size() <= MAX_CONSIDERED_VARIANTS + eps);
        }
    }

};
