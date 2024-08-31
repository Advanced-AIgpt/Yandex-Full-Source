#include <alice/tests/difftest/shooter/library/yav/yav.h>

#include <library/cpp/testing/gmock_in_unittest/gmock.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice::NShooter;
using namespace testing;

namespace {

constexpr auto CORRECT_ANSWER = TStringBuf(R"({
    "status": "ok",
    "version": {
        "comment": "Set S3_STUB_ACCESS for prod version",
        "parent_version_uuid": "ver-01dfdndk9caq71h12ksv5pk6pm",
        "version": "ver-01dj8b3sqsxp2he1e4s9b2xqq0",
        "created_at": 1565795673.85,
        "created_by": 1120000000090139,
        "secret_uuid": "sec-01cnbk6vvm6mfrhdyzamjhm4cm",
        "creator_login": "yakovdom",
        "value": [
            {
                "value": "TOP_SECRET1",
                "key": "TV_CLIENT_ID"
            },
            {
                "value": "TOP_SECRET2",
                "key": "MAPS_STATIC_API_KEY"
            },
            {
                "value": "TOP_SECRET3",
                "key": "VINS_MONGO_PASSWORD"
            },
            {
                "value": "TOP_SECRET4",
                "key": "TV_UID"
            },
            {
                "value": "TOP_SECRET5",
                "key": "XIVA_TOKEN"
            },
            {
                "value": "TOP_SECRET6",
                "key": "TESTENV_TOKEN"
            },
            {
                "value": "TOP_SECRET7",
                "key": "AMEDIATEKA_CLIENT_ID"
            },
            {
                "value": "TOP_SECRET8",
                "key": "WEATHERNC_STATIC_API_KEY"
            },
            {
                "value": "TOP_SECRET9",
                "key": "WEATHER_API_KEY"
            },
            {
                "value": "TOP_SECRET10",
                "key": "YNAVIGATOR_KEY"
            },
            {
                "value": "TOP_SECRET11",
                "key": "SUP_TOKEN"
            },
            {
                "value": "TOP_SECRET12",
                "key": "GOOGLEAPIS_KEY"
            },
            {
                "value": "TOP_SECRET13",
                "key": "TVM2_SECRET"
            },
            {
                "value": "TOP_SECRET14",
                "key": "YDB_TOKEN"
            },
            {
                "value": "TOP_SECRET15",
                "key": "TVM2_ID"
            },
            {
                "value": "TOP_SECRET16",
                "key": "S3_STUB_ACCESS"
            },
            {
                "value": "TOP_SECRET17",
                "key": "AMEDIATEKA_APPLICATION_NAME"
            },
            {
                "value": "TOP_SECRET18",
                "key": "AMEDIATEKA_CLIENT_SECRET"
            }
        ],
        "secret_name": "bass-dev"
    }
})");

class TMockYavRequester : public IYavRequester {
public:
    MOCK_METHOD(TString, Request, (const TString& secretId, const TString& oauthToken), (const, override));
};

Y_UNIT_TEST_SUITE(Yav) {
    Y_UNIT_TEST(Common) {
        TString secretId{"SUPER_SECRET_ID_!@#$%^&*()-"};
        TString oauthToken{"i-am-the-user"};

        {
            StrictMock<TMockYavRequester> requester;
            EXPECT_CALL(requester, Request(secretId, oauthToken))
                .WillRepeatedly(Return(TString{CORRECT_ANSWER}));

            TYav yav{secretId, oauthToken, requester};
            UNIT_ASSERT_EQUAL(yav["TV_CLIENT_ID"], "TOP_SECRET1");
            UNIT_ASSERT_EQUAL(yav["MAPS_STATIC_API_KEY"], "TOP_SECRET2");
            UNIT_ASSERT_EQUAL(yav["VINS_MONGO_PASSWORD"], "TOP_SECRET3");
            UNIT_ASSERT_EQUAL(yav["TVM2_SECRET"], "TOP_SECRET13");
            UNIT_ASSERT_EQUAL(yav["AMEDIATEKA_CLIENT_SECRET"], "TOP_SECRET18");
        }
    }
}

} // namespace
