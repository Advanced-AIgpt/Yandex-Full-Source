#include "constructor.h"

#include <alice/library/json/json.h>
#include <alice/library/unittest/message_diff.h>

#include <alice/megamind/protos/common/location.pb.h>
#include <alice/protos/data/contacts.pb.h>

#include <library/cpp/testing/unittest/registar.h>

#include <utility>

namespace {

using namespace NAlice;
using namespace NAlice::NMegamindApi;


TSpeechKitRequestProto MakeRequest(const TStringBuf rawJsonRequest) {
    TRequestConstructor constructor{TRTLogger::StderrLogger()};
    UNIT_ASSERT(constructor.PushSpeechKitJson(JsonFromString(rawJsonRequest)).Ok());
    return std::move(constructor).MakeRequest();
}

Y_UNIT_TEST_SUITE(RequestConstructor) {
    Y_UNIT_TEST(DefaultLbsAccuracy) {
        TRequestConstructor constructor{};
        constructor.PushSpeechKitJson(JsonFromString(TStringBuf{R"({
            "request": {
                "location": { "lat": 1 }
            }
        })"}));
        UNIT_ASSERT_DOUBLES_EQUAL(std::move(constructor).MakeRequest().GetRequest().GetLocation().GetAccuracy(), 140,
                                  1e-8);
    }

    Y_UNIT_TEST(DefaultLbsAccuracyDoesNotOverwrite) {
        TRequestConstructor constructor{};
        constructor.PushSpeechKitJson(JsonFromString(TStringBuf{R"({
            "request": {
                "location": { "accuracy": 1 }
            }
        })"}));
        UNIT_ASSERT_DOUBLES_EQUAL(std::move(constructor).MakeRequest().GetRequest().GetLocation().GetAccuracy(), 1,
                                  1e-8);
    }

    Y_UNIT_TEST(FixVideoCurrentScreen) {
        const auto fixed = MakeRequest(TStringBuf{R"({
            "request": {
                "device_state": {
                    "video": {
                        "current_screen": 18
                    }
                }
            }
        })"});
        const auto origin = MakeRequest(TStringBuf{R"({
            "request": {
                "device_state": {
                    "video": {
                    }
                }
            }
        })"});
        UNIT_ASSERT_MESSAGES_EQUAL(fixed, origin);
    }

    Y_UNIT_TEST(ParseRawDeviceState) {
        const auto actual = MakeRequest(TStringBuf{R"({
            "request": {
                "device_state_raw": "Cglxd2Vxd2Vxd2U="
            }
        })"});
        const auto expected = MakeRequest(TStringBuf{R"({
            "request": {
                "device_state": {
                    "device_id": "qweqweqwe"
                }
            }
        })"});
        Cout << "Expected: " << expected << Endl;
        Cout << "Actual: " << actual << Endl;
        UNIT_ASSERT_MESSAGES_EQUAL(expected, actual);
    }

    Y_UNIT_TEST(ParseRawEnvironmentState) {
        const auto actual = MakeRequest(TStringBuf{R"({
            "request": {
                "environment_state_raw": "GgsKCXF3ZXF3ZXF3ZQ=="
            }
        })"});
        const auto expected = MakeRequest(TStringBuf{R"({
            "request": {
                "environment_state": {
                    "endpoints": [{
                        "id": "qweqweqwe"
                    }]
                }
            }
        })"});
        UNIT_ASSERT_MESSAGES_EQUAL(expected, actual);
    }

    Y_UNIT_TEST(ParseContactsProto) {
        TRequestConstructor constructor{};
        constructor.PushSpeechKitJson(JsonFromString(TStringBuf{R"({
            "contacts_proto": "Co0HCpICChFXaGF0c0FwcCBCdXNpbmVzcxIQY29tLndoYXRzYXBwLnc0YhoZ0J7Qu9GM0LPQsCDQk9GA0LjRiNC40L3QsCIK0J7Qu9GM0LPQsDIO0JPRgNC40YjQuNC90LA4_R1A0mRKrQEzMDU2cjUwLTI3OEE2MkU4MTAwNjEwOTY0NkRBNDY3ODA2LjM1NjFpNjQ1MzE5NzM4ZTg0OTYwMi4zNzg5cjE2MjAtMjc4QTYyRTgxMDA2MTA5NjQ2REE0Njc4MDYuOTAycjI1OTQtMjc4QTYyRTgxMDA2MTA5NjQ2REE0Njc4MDYuMjQ4cjMwNTUtMjc4QTYyRTgxMDA2MTA5NjQ2REE0Njc4MDYuMzY3NWk1MAqLAgoIV2hhdHNBcHASDGNvbS53aGF0c2FwcBoh0KHQstC10YLQu9Cw0L3QsCDQodC40YLQuNC70LjQvdC6IhDQodCy0LXRgtC70LDQvdCwMhDQodC40YLQuNC70LjQvdC6OPsWQLcoSqMBMzA1NnI0NjQtMjc5QzBFMjhBMjYyMDY3ODA2OUM0NkEyNDY2MjQ2Nzg1NC4zNTYxaTE0NmUxOTBiOGM3ZDZjZDIuMzc4OXIxNDAzLTI3OUMwRTI4QTI2MjA2NzgwNjlDNDZBMjQ2NjI0Njc4NTQuMjQ4cjI5MjItMjc5QzBFMjhBMjYyMDY3ODA2OUM0NkEyNDY2MjQ2Nzg1NC4zNjc1aTQ2NBKKAQioJBIKY29tLmdvb2dsZRpgMzA1NnI0NTUtMjc5QzBDMjg5NjBDMDY3ODU0NTQwNjFBOTZFNkEyNTQwNkM4MjgwRTA2QTIwNkEyRThGNDc4MDYuMzU2MWk0NjY2NDEzMzhhN2Y1MzdjLjM2NzVpNDU1Ig84IDk4NSA3MDQtNDUtMzUqBm1vYmlsZRLaAQillwESDmNvbS52aWJlci52b2lwGq4BMzA1NnIzOTUtMjc5QzYyMjhBMjA2QTJFODk2QUM4QTBDNzguMzU2MWlkMjQ4YTUxOGZiYWExN2IuMzc4OXIxNTA3LTI3OUM2MjI4QTIwNkEyRTg5NkFDOEEwQzc4LjkwMnIyNjgxLTI3OUM2MjI4QTIwNkEyRTg5NkFDOEEwQzc4LjI0OHIzMDI4LTI3OUM2MjI4QTIwNkEyRTg5NkFDOEEwQzc4LjM2NzVpMzk1IgwrNzk2MjE3MzAwNzEqBXZpYmVyEgJvaw,,"
        })"}));
        const auto request = std::move(constructor).MakeRequest();
        UNIT_ASSERT_EQUAL(request.GetContacts().GetStatus(), "ok");
        UNIT_ASSERT_EQUAL(request.GetContacts().GetData().GetContacts().size(), 2);
        UNIT_ASSERT_EQUAL(request.GetContacts().GetData().GetPhones().size(), 2);
    }

    Y_UNIT_TEST(LeaveContacts) {
        TRequestConstructor constructor{};
        constructor.PushSpeechKitJson(JsonFromString(TStringBuf{R"({
            "contacts": {
                "status": "ok",
                "data": {
                    "contacts": [
                        {
                            "account_name": "test@gmail.com",
                            "first_name": "Test",
                            "contact_id": "123",
                            "lookup_key": "123",
                            "account_type": "com.google",
                            "display_name": "Test"
                        }
                    ]
                }
            },
            "contacts_proto": "Co0HCpICChFXaGF0c0FwcCBCdXNpbmVzcxIQY29tLndoYXRzYXBwLnc0YhoZ0J7Qu9GM0LPQsCDQk9GA0LjRiNC40L3QsCIK0J7Qu9GM0LPQsDIO0JPRgNC40YjQuNC90LA4_R1A0mRKrQEzMDU2cjUwLTI3OEE2MkU4MTAwNjEwOTY0NkRBNDY3ODA2LjM1NjFpNjQ1MzE5NzM4ZTg0OTYwMi4zNzg5cjE2MjAtMjc4QTYyRTgxMDA2MTA5NjQ2REE0Njc4MDYuOTAycjI1OTQtMjc4QTYyRTgxMDA2MTA5NjQ2REE0Njc4MDYuMjQ4cjMwNTUtMjc4QTYyRTgxMDA2MTA5NjQ2REE0Njc4MDYuMzY3NWk1MAqLAgoIV2hhdHNBcHASDGNvbS53aGF0c2FwcBoh0KHQstC10YLQu9Cw0L3QsCDQodC40YLQuNC70LjQvdC6IhDQodCy0LXRgtC70LDQvdCwMhDQodC40YLQuNC70LjQvdC6OPsWQLcoSqMBMzA1NnI0NjQtMjc5QzBFMjhBMjYyMDY3ODA2OUM0NkEyNDY2MjQ2Nzg1NC4zNTYxaTE0NmUxOTBiOGM3ZDZjZDIuMzc4OXIxNDAzLTI3OUMwRTI4QTI2MjA2NzgwNjlDNDZBMjQ2NjI0Njc4NTQuMjQ4cjI5MjItMjc5QzBFMjhBMjYyMDY3ODA2OUM0NkEyNDY2MjQ2Nzg1NC4zNjc1aTQ2NBKKAQioJBIKY29tLmdvb2dsZRpgMzA1NnI0NTUtMjc5QzBDMjg5NjBDMDY3ODU0NTQwNjFBOTZFNkEyNTQwNkM4MjgwRTA2QTIwNkEyRThGNDc4MDYuMzU2MWk0NjY2NDEzMzhhN2Y1MzdjLjM2NzVpNDU1Ig84IDk4NSA3MDQtNDUtMzUqBm1vYmlsZRLaAQillwESDmNvbS52aWJlci52b2lwGq4BMzA1NnIzOTUtMjc5QzYyMjhBMjA2QTJFODk2QUM4QTBDNzguMzU2MWlkMjQ4YTUxOGZiYWExN2IuMzc4OXIxNTA3LTI3OUM2MjI4QTIwNkEyRTg5NkFDOEEwQzc4LjkwMnIyNjgxLTI3OUM2MjI4QTIwNkEyRTg5NkFDOEEwQzc4LjI0OHIzMDI4LTI3OUM2MjI4QTIwNkEyRTg5NkFDOEEwQzc4LjM2NzVpMzk1IgwrNzk2MjE3MzAwNzEqBXZpYmVyEgJvaw,,"
        })"}));
        const auto request = std::move(constructor).MakeRequest();
        UNIT_ASSERT_EQUAL(request.GetContacts().GetStatus(), "ok");
        UNIT_ASSERT_EQUAL(request.GetContacts().GetData().GetContacts().size(), 1);
        UNIT_ASSERT_EQUAL(request.GetContacts().GetData().GetPhones().size(), 0);
    }

    Y_UNIT_TEST(ParseBadContactsProto) {
        TRequestConstructor constructor{};
        constructor.PushSpeechKitJson(JsonFromString(TStringBuf{R"({
            "contacts_proto": "YXNk"
        })"}));
        const auto request = std::move(constructor).MakeRequest();
        UNIT_ASSERT(!request.HasContacts());
    }

    Y_UNIT_TEST(ParseEmptyContactsProto) {
        TRequestConstructor constructor{};
        constructor.PushSpeechKitJson(JsonFromString(TStringBuf{R"({
            "contacts_proto": ""
        })"}));
        const auto request = std::move(constructor).MakeRequest();
        UNIT_ASSERT(!request.HasContacts());
    }

    Y_UNIT_TEST(ParseNotExistContactsProto) {
        TRequestConstructor constructor{};
        constructor.PushSpeechKitJson(JsonFromString(TStringBuf{R"({
        })"}));
        const auto request = std::move(constructor).MakeRequest();
        UNIT_ASSERT(!request.HasContacts());
    }

    Y_UNIT_TEST(PatchContactsOk) {
        TSpeechKitRequestProto proto;
        proto.SetContactsProto("Co0HCpICChFXaGF0c0FwcCBCdXNpbmVzcxIQY29tLndoYXRzYXBwLnc0YhoZ0J7Qu9GM0LPQsCDQk9GA0LjRiNC40L3QsCIK0J7Qu9GM0LPQsDIO0JPRgNC40YjQuNC90LA4_R1A0mRKrQEzMDU2cjUwLTI3OEE2MkU4MTAwNjEwOTY0NkRBNDY3ODA2LjM1NjFpNjQ1MzE5NzM4ZTg0OTYwMi4zNzg5cjE2MjAtMjc4QTYyRTgxMDA2MTA5NjQ2REE0Njc4MDYuOTAycjI1OTQtMjc4QTYyRTgxMDA2MTA5NjQ2REE0Njc4MDYuMjQ4cjMwNTUtMjc4QTYyRTgxMDA2MTA5NjQ2REE0Njc4MDYuMzY3NWk1MAqLAgoIV2hhdHNBcHASDGNvbS53aGF0c2FwcBoh0KHQstC10YLQu9Cw0L3QsCDQodC40YLQuNC70LjQvdC6IhDQodCy0LXRgtC70LDQvdCwMhDQodC40YLQuNC70LjQvdC6OPsWQLcoSqMBMzA1NnI0NjQtMjc5QzBFMjhBMjYyMDY3ODA2OUM0NkEyNDY2MjQ2Nzg1NC4zNTYxaTE0NmUxOTBiOGM3ZDZjZDIuMzc4OXIxNDAzLTI3OUMwRTI4QTI2MjA2NzgwNjlDNDZBMjQ2NjI0Njc4NTQuMjQ4cjI5MjItMjc5QzBFMjhBMjYyMDY3ODA2OUM0NkEyNDY2MjQ2Nzg1NC4zNjc1aTQ2NBKKAQioJBIKY29tLmdvb2dsZRpgMzA1NnI0NTUtMjc5QzBDMjg5NjBDMDY3ODU0NTQwNjFBOTZFNkEyNTQwNkM4MjgwRTA2QTIwNkEyRThGNDc4MDYuMzU2MWk0NjY2NDEzMzhhN2Y1MzdjLjM2NzVpNDU1Ig84IDk4NSA3MDQtNDUtMzUqBm1vYmlsZRLaAQillwESDmNvbS52aWJlci52b2lwGq4BMzA1NnIzOTUtMjc5QzYyMjhBMjA2QTJFODk2QUM4QTBDNzguMzU2MWlkMjQ4YTUxOGZiYWExN2IuMzc4OXIxNTA3LTI3OUM2MjI4QTIwNkEyRTg5NkFDOEEwQzc4LjkwMnIyNjgxLTI3OUM2MjI4QTIwNkEyRTg5NkFDOEEwQzc4LjI0OHIzMDI4LTI3OUM2MjI4QTIwNkEyRTg5NkFDOEEwQzc4LjM2NzVpMzk1IgwrNzk2MjE3MzAwNzEqBXZpYmVyEgJvaw,,");
        TRequestConstructor::PatchContacts(proto, TRTLogger::StderrLogger());
        UNIT_ASSERT_VALUES_EQUAL(proto.GetContacts().GetStatus(), "ok");
        UNIT_ASSERT(!proto.HasContactsProto());
    }

    Y_UNIT_TEST(PatchContactsEmptyProto) {
        TSpeechKitRequestProto proto;
        TRequestConstructor::PatchContacts(proto, TRTLogger::StderrLogger());
        UNIT_ASSERT(!proto.HasContactsProto());
        UNIT_ASSERT(!proto.HasContacts());
    }

    Y_UNIT_TEST(PatchContactsNotOverride) {
        TSpeechKitRequestProto proto;
        proto.MutableContacts()->SetStatus("ok");
        proto.SetContactsProto("Co0HCpICChFXaGF0c0FwcCBCdXNpbmVzcxIQY29tLndoYXRzYXBwLnc0YhoZ0J7Qu9GM0LPQsCDQk9GA0LjRiNC40L3QsCIK0J7Qu9GM0LPQsDIO0JPRgNC40YjQuNC90LA4_R1A0mRKrQEzMDU2cjUwLTI3OEE2MkU4MTAwNjEwOTY0NkRBNDY3ODA2LjM1NjFpNjQ1MzE5NzM4ZTg0OTYwMi4zNzg5cjE2MjAtMjc4QTYyRTgxMDA2MTA5NjQ2REE0Njc4MDYuOTAycjI1OTQtMjc4QTYyRTgxMDA2MTA5NjQ2REE0Njc4MDYuMjQ4cjMwNTUtMjc4QTYyRTgxMDA2MTA5NjQ2REE0Njc4MDYuMzY3NWk1MAqLAgoIV2hhdHNBcHASDGNvbS53aGF0c2FwcBoh0KHQstC10YLQu9Cw0L3QsCDQodC40YLQuNC70LjQvdC6IhDQodCy0LXRgtC70LDQvdCwMhDQodC40YLQuNC70LjQvdC6OPsWQLcoSqMBMzA1NnI0NjQtMjc5QzBFMjhBMjYyMDY3ODA2OUM0NkEyNDY2MjQ2Nzg1NC4zNTYxaTE0NmUxOTBiOGM3ZDZjZDIuMzc4OXIxNDAzLTI3OUMwRTI4QTI2MjA2NzgwNjlDNDZBMjQ2NjI0Njc4NTQuMjQ4cjI5MjItMjc5QzBFMjhBMjYyMDY3ODA2OUM0NkEyNDY2MjQ2Nzg1NC4zNjc1aTQ2NBKKAQioJBIKY29tLmdvb2dsZRpgMzA1NnI0NTUtMjc5QzBDMjg5NjBDMDY3ODU0NTQwNjFBOTZFNkEyNTQwNkM4MjgwRTA2QTIwNkEyRThGNDc4MDYuMzU2MWk0NjY2NDEzMzhhN2Y1MzdjLjM2NzVpNDU1Ig84IDk4NSA3MDQtNDUtMzUqBm1vYmlsZRLaAQillwESDmNvbS52aWJlci52b2lwGq4BMzA1NnIzOTUtMjc5QzYyMjhBMjA2QTJFODk2QUM4QTBDNzguMzU2MWlkMjQ4YTUxOGZiYWExN2IuMzc4OXIxNTA3LTI3OUM2MjI4QTIwNkEyRTg5NkFDOEEwQzc4LjkwMnIyNjgxLTI3OUM2MjI4QTIwNkEyRTg5NkFDOEEwQzc4LjI0OHIzMDI4LTI3OUM2MjI4QTIwNkEyRTg5NkFDOEEwQzc4LjM2NzVpMzk1IgwrNzk2MjE3MzAwNzEqBXZpYmVyEgJvaw,,");
        TRequestConstructor::PatchContacts(proto, TRTLogger::StderrLogger());
        UNIT_ASSERT(proto.HasContactsProto());
        UNIT_ASSERT(proto.HasContacts());
        UNIT_ASSERT(!proto.GetContacts().HasData());
    }
}

} // namespace
