#include <alice/wonderlogs/library/robot/account_type.h>

#include <alice/library/json/json.h>
#include <alice/library/unittest/message_diff.h>
#include <alice/megamind/protos/speechkit/request.pb.h>

#include <google/protobuf/struct.pb.h>
#include <google/protobuf/text_format.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NWonderlogs {

Y_UNIT_TEST_SUITE(AccountType) {
    Y_UNIT_TEST(AccountType) {
        const TString UUID = "uuid";
        const TString MESSAGE_ID = "message_id";
        const TString MEGAMIND_REQUEST_ID = "megamind_request_id";
        const TString PROCESS_ID = "process_id";

        {
            TAccountTypeInfo accountTypeInfo{UUID,
                                             MESSAGE_ID,
                                             MEGAMIND_REQUEST_ID,
                                             /* processId= */ PROCESS_ID,
                                             /* locationYandexNet= */ {},
                                             /* locationYandexStaff= */ {},
                                             /* yandexNet= */ {},
                                             /* yandexStaff */ {}};

            UNIT_ASSERT(accountTypeInfo.Robot());
            UNIT_ASSERT_EQUAL(TWonderlog::AT_ROBOT, accountTypeInfo.Type());

            accountTypeInfo.ProcessId = {};
            UNIT_ASSERT(!accountTypeInfo.Robot());

            accountTypeInfo.Uuid = "deadbeef715f43ddae6fb46a544ea5ef";
            UNIT_ASSERT(accountTypeInfo.Robot());
            accountTypeInfo.Uuid = UUID;

            accountTypeInfo.MegamindRequestId = "ffffffff-ffff-ffff-3476-24e6bd1ff8f0";
            UNIT_ASSERT(accountTypeInfo.Robot());
            accountTypeInfo.MegamindRequestId = MEGAMIND_REQUEST_ID;

            accountTypeInfo.MessageId = "ffffffff-ffff-ffff-3476-24e6bd1ff8f0";
            UNIT_ASSERT(accountTypeInfo.Robot());
            accountTypeInfo.MessageId = MESSAGE_ID;

            accountTypeInfo.LocationYandexNet = true;
            UNIT_ASSERT(accountTypeInfo.Robot());

            accountTypeInfo.YandexNet = true;
            UNIT_ASSERT(accountTypeInfo.Robot());

            accountTypeInfo.LocationYandexNet = false;
            UNIT_ASSERT(accountTypeInfo.Robot());

            accountTypeInfo.YandexNet = false;
            UNIT_ASSERT(!accountTypeInfo.Robot());
        }

        {
            TAccountTypeInfo accountTypeInfo{UUID,
                                             MESSAGE_ID,
                                             MEGAMIND_REQUEST_ID,
                                             /* processId= */ {},
                                             /* locationYandexNet= */ {},
                                             /* locationYandexStaff= */ {},
                                             /* yandexNet= */ {},
                                             /* yandexStaff */ {}};

            UNIT_ASSERT(!accountTypeInfo.Robot());

            accountTypeInfo.LocationYandexStaff = true;
            UNIT_ASSERT(accountTypeInfo.Staff());
            UNIT_ASSERT(!accountTypeInfo.Robot());
            UNIT_ASSERT_UNEQUAL(TWonderlog::AT_ROBOT, accountTypeInfo.Type());
            UNIT_ASSERT_EQUAL(TWonderlog::AT_STAFF, accountTypeInfo.Type());

            accountTypeInfo.YandexStaff = true;
            UNIT_ASSERT(accountTypeInfo.Staff());
            UNIT_ASSERT(!accountTypeInfo.Robot());
            UNIT_ASSERT_UNEQUAL(TWonderlog::AT_ROBOT, accountTypeInfo.Type());
            UNIT_ASSERT_EQUAL(TWonderlog::AT_STAFF, accountTypeInfo.Type());

            accountTypeInfo.LocationYandexStaff = false;
            UNIT_ASSERT(accountTypeInfo.Staff());
            UNIT_ASSERT(!accountTypeInfo.Robot());
            UNIT_ASSERT_UNEQUAL(TWonderlog::AT_ROBOT, accountTypeInfo.Type());
            UNIT_ASSERT_EQUAL(TWonderlog::AT_STAFF, accountTypeInfo.Type());

            accountTypeInfo.YandexStaff = false;
            UNIT_ASSERT(!accountTypeInfo.Staff());

            accountTypeInfo.LocationYandexNet = true;
            accountTypeInfo.LocationYandexStaff = true;

            UNIT_ASSERT(accountTypeInfo.Staff());
            UNIT_ASSERT(!accountTypeInfo.Robot());
            UNIT_ASSERT_UNEQUAL(TWonderlog::AT_ROBOT, accountTypeInfo.Type());
            UNIT_ASSERT_EQUAL(TWonderlog::AT_STAFF, accountTypeInfo.Type());

            accountTypeInfo.YandexStaff = true;
            UNIT_ASSERT(accountTypeInfo.Staff());
            UNIT_ASSERT(!accountTypeInfo.Robot());
            UNIT_ASSERT_UNEQUAL(TWonderlog::AT_ROBOT, accountTypeInfo.Type());
            UNIT_ASSERT_EQUAL(TWonderlog::AT_STAFF, accountTypeInfo.Type());

            accountTypeInfo.LocationYandexStaff = false;
            UNIT_ASSERT(accountTypeInfo.Staff());
            UNIT_ASSERT(!accountTypeInfo.Robot());
            UNIT_ASSERT_UNEQUAL(TWonderlog::AT_ROBOT, accountTypeInfo.Type());
            UNIT_ASSERT_EQUAL(TWonderlog::AT_STAFF, accountTypeInfo.Type());
        }

        {
            TAccountTypeInfo accountTypeInfo{UUID,
                                             MESSAGE_ID,
                                             MEGAMIND_REQUEST_ID,
                                             /* processId= */ {},
                                             /* locationYandexNet= */ {},
                                             /* locationYandexStaff= */ {},
                                             /* yandexNet= */ {},
                                             /* yandexStaff */ {}};

            UNIT_ASSERT_EQUAL(TWonderlog::AT_HUMAN, accountTypeInfo.Type());
        }
    }

    Y_UNIT_TEST(AccountTypeConstructor) {
        {
            const TString UUID = "uuid";
            const TString MESSAGE_ID = "message_id";
            const TString MEGAMIND_REQUEST_ID = "megamind_request_id";
            const TString PROCESS_ID = "process_id";

            TWonderlog wonderlog;
            wonderlog.SetUuid(UUID);
            wonderlog.SetMessageId(MESSAGE_ID);
            wonderlog.SetMegamindRequestId(MEGAMIND_REQUEST_ID);
            wonderlog.MutableDownloadingInfo()->MutableUniproxy()->SetYandexNet(true);
            wonderlog.MutableDownloadingInfo()->MutableUniproxy()->SetStaffNet(false);
            wonderlog.MutableSpeechkitRequest()
                ->MutableRequest()
                ->MutableAdditionalOptions()
                ->MutableBassOptions()
                ->SetProcessId(PROCESS_ID);
            {
                auto& location =
                    *wonderlog.MutableSpeechkitRequest()->MutableRequest()->MutableLaasRegion()->mutable_fields();
                location["is_yandex_net"].set_bool_value(true);
                location["is_yandex_staff"].set_bool_value(false);
            }
            {
                TAccountTypeInfo accountTypeInfo(wonderlog);

                UNIT_ASSERT_EQUAL(UUID, accountTypeInfo.Uuid);
                UNIT_ASSERT_EQUAL(MESSAGE_ID, accountTypeInfo.MessageId);
                UNIT_ASSERT_EQUAL(MEGAMIND_REQUEST_ID, accountTypeInfo.MegamindRequestId);

                UNIT_ASSERT(accountTypeInfo.ProcessId);
                UNIT_ASSERT_EQUAL(PROCESS_ID, *accountTypeInfo.ProcessId);

                UNIT_ASSERT(accountTypeInfo.LocationYandexNet);
                UNIT_ASSERT(*accountTypeInfo.LocationYandexNet);
                UNIT_ASSERT(accountTypeInfo.LocationYandexStaff);
                UNIT_ASSERT(!*accountTypeInfo.LocationYandexStaff);

                UNIT_ASSERT(accountTypeInfo.YandexNet);
                UNIT_ASSERT(*accountTypeInfo.YandexNet);
                UNIT_ASSERT(accountTypeInfo.YandexStaff);
                UNIT_ASSERT(!*accountTypeInfo.YandexStaff);
            }

            {
                auto& location =
                    *wonderlog.MutableSpeechkitRequest()->MutableRequest()->MutableLaasRegion()->mutable_fields();
                location["is_yandex_net"].set_number_value(1337);
                location["is_yandex_staff"].set_string_value("qq");
            }

            {
                TAccountTypeInfo accountTypeInfo(wonderlog);

                UNIT_ASSERT(!accountTypeInfo.LocationYandexNet);
                UNIT_ASSERT(!accountTypeInfo.LocationYandexStaff);
            }
        }
    }
}

} // namespace NAlice::NWonderlogs
