#include <alice/library/contacts/contacts.h>
#include <alice/megamind/protos/common/frame.pb.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/protobuf/json/proto2json.h>
#include <library/cpp/protobuf/json/json2proto.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <util/random/shuffle.h>
#include <util/stream/str.h>
#include <util/string/split.h>
#include <util/stream/file.h>
#include <util/system/hp_timer.h>


using namespace NProtobufJson;

namespace {
    struct TContactsAppletOptions {
        TString InputPath;
        size_t BookSize;
        size_t MeasurePerformanceCount;
        bool Debug;
    };

    TContactsAppletOptions ReadContactsAppletOptions(int argc, const char** argv) {
        TContactsAppletOptions result;

        auto parser = NLastGetopt::TOpts();

        parser.AddLongOption('i', "input-path")
            .StoreResult(&result.InputPath)
            .Help("Input file path.");
        
        parser.AddLongOption('s', "book-size")
            .StoreResult(&result.BookSize)
            .DefaultValue(2000)
            .Help("Random-generated address book size. Used when input-path is not provided.");
        
        parser.AddLongOption('t', "time")
            .StoreResult(&result.MeasurePerformanceCount)
            .DefaultValue(0)
            .Help("Measure performance iterations count.");
        
        parser.AddLongOption('d', "debug")
            .StoreTrue(&result.Debug)
            .Help("Debug flag for additional output.");

        parser.AddHelpOption();
        parser.SetFreeArgsNum(0);
        NLastGetopt::TOptsParseResult parserResult{&parser, argc, argv};
        
        return result;
    }

    void ReadContactsFromJson(const TString& filePath, NAlice::NData::TContactsList& contactsList) {
        TFileInput in(filePath);
        NJson::TJsonValue contactsJson;
        ReadJsonTree(&in, &contactsJson, /* throwOnError= */ true);
        Json2Proto(contactsJson["data"], contactsList, TJson2ProtoConfig().SetUseJsonName(true));
    }

    void GenerateRandomContacts(size_t numContacts, NAlice::NData::TContactsList& contactsList) {
        contactsList.SetIsKnownUuid(true);
        TString lookupKey = "3056r5210-278E06A246030827B68AC828A2628A728A788A9C030827.8E968A9C8";
        TString displayName = "abcdef ghijklm nopqrstuvwxyz 4 5 6 7 8 9";
        TString phoneNumber = "0 123 456 78 91";
        const TString accountType = "com.oppo.contacts.device";
        for (size_t i = 0; i < numContacts; ++i) {
            auto contact = contactsList.AddContacts();
            auto phone = contactsList.AddPhones();
            Shuffle(lookupKey.begin(), lookupKey.end());
            contact->SetLookupKey(lookupKey);
            phone->SetLookupKey(lookupKey);
            Shuffle(displayName.begin(), displayName.end());
            contact->SetDisplayName(displayName);
            contact->SetAccountType(accountType);
            phone->SetAccountType(accountType);
            contact->SetContactId(i);
            phone->SetId(i);
            phone->SetType("mobile");
            Shuffle(phoneNumber.begin(), phoneNumber.end());
            phone->SetPhone(phoneNumber);
        }
    }

    NAlice::TClientEntityList GetContactsEntityList(NAlice::NData::TContactsList& contactsList) {
        NAlice::TClientEntity contactsEntity;
        contactsEntity.SetName("contacts");

        for (const auto &contact : contactsList.GetContacts()) {
            NAlice::TNluHint nluHint;
            auto& instance = *nluHint.AddInstances();
            instance.SetPhrase(NAlice::NContacts::GetContactMatchingInfo(contact));
            (*contactsEntity.MutableItems())[NAlice::NContacts::GetContactUniqueKey(contact)] = std::move(nluHint);
        }

        NAlice::TClientEntityList contactsEntityList;
        *contactsEntityList.AddEntities() = std::move(contactsEntity);

        return contactsEntityList;
    }

    static int RunContactsApplet(int argc, const char** argv) {
        TContactsAppletOptions options = ReadContactsAppletOptions(argc, argv);

        NAlice::NData::TContactsList contactsList;
        if (options.InputPath) {
            ReadContactsFromJson(options.InputPath, contactsList);
        } else {
            GenerateRandomContacts(options.BookSize, contactsList);
        }

        const auto contactsEntityList = GetContactsEntityList(contactsList);
        if (options.Debug) {
            Cout << Proto2Json(contactsEntityList) << Endl;
        }
        Cout << Base64EncodeUrl(contactsEntityList.SerializeAsString()) << Endl;

        if (options.MeasurePerformanceCount != 0) {
            double max = 0;
            double sum = 0;
            for (size_t it = 0; it < options.MeasurePerformanceCount; ++it) {
                THPTimer watcher;
                const auto contactsEntityList = GetContactsEntityList(contactsList);
                double time = watcher.PassedReset();
                max = std::max(time, max);
                sum += time;
            }
            Cout << "Size: " << contactsList.GetContacts().size() << Endl;
            Cout << "max: " << max * 1000 << "ms" << Endl;
            Cout << "avg: " << sum * 1000 / options.MeasurePerformanceCount << "ms" << Endl;
        }

        return 0;
    }

} // namespace

int main(int argc, const char** argv) {
    try {
        return RunContactsApplet(argc, argv);
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
        return 1;
    }
}
