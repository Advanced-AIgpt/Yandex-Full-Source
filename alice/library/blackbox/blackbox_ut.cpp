#include "blackbox.h"

#include <alice/library/network/headers.h>
#include <alice/library/unittest/fake_fetcher.h>

#include <library/cpp/testing/gmock_in_unittest/gmock.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/stream/str.h>

namespace {

using namespace NAlice;

Y_UNIT_TEST_SUITE(BlackBox) {
    Y_UNIT_TEST(ApiPrepareRequestParams) {
        {
            NAlice::NTestingHelpers::TFakeRequestBuilder builder;
            auto status = PrepareBlackBoxRequest(builder, "client-ip", "user-auth-token",
                                                 EBlackBoxPrepareParam::NeedUserInfo);
            UNIT_ASSERT(!status.Defined());
            UNIT_ASSERT_STRINGS_EQUAL(
                builder.Cgi.Print(),
                "aliases=13&attributes=1015%2C8&dbfields=userinfo.firstname.uid%2Cuserinfo.lastname.uid"
                "&emails=getdefault&format=json&get_billing_features=all&get_login_id=yes&getphones=bound"
                "&method=oauth&phone_attributes=107%2C102%2C108&userip=client-ip"
            );
            UNIT_ASSERT(builder.HasHeader(NNetwork::HEADER_AUTHORIZATION, "oauth user-auth-token"));
        }

        {
            NAlice::NTestingHelpers::TFakeRequestBuilder builder;
            auto status = PrepareBlackBoxRequest(builder, "client-ip", "user-auth-token", EBlackBoxPrepareParam::NeedTicket);
            UNIT_ASSERT(!status.Defined());
            UNIT_ASSERT_STRINGS_EQUAL(builder.Cgi.Print(),
                                      "format=json&get_user_ticket=yes&method=oauth&userip=client-ip");
            UNIT_ASSERT(builder.HasHeader(NNetwork::HEADER_AUTHORIZATION, "oauth user-auth-token"));
        }
    }

    Y_UNIT_TEST(ApiPrepareRequestTokenSmoke) {
        NAlice::NTestingHelpers::TFakeRequestBuilder builder;

        struct TItem {
            TString Source;
            TString Canonized;
        };
        const TItem items[] = {
            { "user-auth-token", "oauth user-auth-token" },
            { "oauth user-auth-token", "oauth user-auth-token" },
            { "OAuth user-auth-token", "oauth user-auth-token" },
        };

        for (const auto& item : items) {
            auto status = PrepareBlackBoxRequest(builder, "client-ip", item.Source);
            UNIT_ASSERT(!status.Defined());
            UNIT_ASSERT(builder.HasHeader(NNetwork::HEADER_AUTHORIZATION, item.Canonized));
        }
    }

    Y_UNIT_TEST(ApiPrepareRequestSmoke) {
        {
            NAlice::NTestingHelpers::TFakeRequestBuilder builder;
            auto status = PrepareBlackBoxRequest(builder, "client-ip", "user-auth-token");

            UNIT_ASSERT(!status.Defined());
            UNIT_ASSERT_STRINGS_EQUAL(
                builder.Cgi.Print(),
                "aliases=13&attributes=1015%2C8&dbfields=userinfo.firstname.uid%2Cuserinfo.lastname.uid"
                "&emails=getdefault&format=json&get_billing_features=all&get_login_id=yes&get_user_ticket=yes&getphones=bound"
                "&method=oauth&phone_attributes=107%2C102%2C108&userip=client-ip"
            );
            UNIT_ASSERT(builder.HasHeader(NNetwork::HEADER_AUTHORIZATION, "oauth user-auth-token"));
        }

        {
            NAlice::NTestingHelpers::TFakeRequestBuilder builder;
            auto status = PrepareBlackBoxRequest(builder, "client-ip", /* authToken= */Nothing());
            UNIT_ASSERT_C(status.Defined(), "no auth token");
        }

        {
            NAlice::NTestingHelpers::TFakeRequestBuilder builder;
            auto status = PrepareBlackBoxRequest(builder, "client-ip", /* authToken= */Nothing(), EBlackBoxPrepareParam::NeedTicket | EBlackBoxPrepareParam::NeedUserInfo);
            UNIT_ASSERT(!status.Defined());

            UNIT_ASSERT(!status.Defined());
            UNIT_ASSERT_STRINGS_EQUAL(
                builder.Cgi.Print(),
                "aliases=13&attributes=1015%2C8&dbfields=userinfo.firstname.uid%2Cuserinfo.lastname.uid"
                "&emails=getdefault&format=json&get_billing_features=all&get_login_id=yes&get_user_ticket=yes&getphones=bound"
                "&method=oauth&phone_attributes=107%2C102%2C108&userip=client-ip");
            UNIT_ASSERT(!builder.HasHeader(NNetwork::HEADER_AUTHORIZATION, "user-auth-token"));
        }

        {
            NAlice::NTestingHelpers::TFakeRequestBuilder builder;
            auto status = PrepareBlackBoxRequest(builder, /* clientIp= */"", /* authToken= */Nothing());
            UNIT_ASSERT(status.Defined());
            UNIT_ASSERT_EQUAL(status->Code(), EBlackBoxErrorCode::NoUserIP);
        }
    }

    Y_UNIT_TEST(ApiParseResponseEmptyValidJson) {
        {
            auto rval = TBlackBoxApi{}.ParseUid("{}");
            UNIT_ASSERT(!rval.IsSuccess());
            UNIT_ASSERT_EQUAL_C(rval.GetError()->Code(), EBlackBoxErrorCode::NoUid, ToString(rval.GetError()->Code()));
        }

        {
            auto rval = TBlackBoxApi{}.ParseTvm2UserTicket("{}");
            UNIT_ASSERT(!rval.IsSuccess());
            UNIT_ASSERT_EQUAL_C(rval.GetError()->Code(), EBlackBoxErrorCode::NoUserTicket, ToString(rval.GetError()->Code()));
        }

        {
            auto rval = TBlackBoxApi{}.ParseFullInfo("{}");
            UNIT_ASSERT(!rval.IsSuccess());
            UNIT_ASSERT_EQUAL_C(rval.GetError()->Code(), EBlackBoxErrorCode::NoUid, ToString(rval.GetError()->Code()));
        }
    }

    Y_UNIT_TEST(ApiParseResponseInvalidJson) {
        {
            auto rval = TBlackBoxApi{}.ParseUid("{");
            UNIT_ASSERT(!rval.IsSuccess());
            UNIT_ASSERT_EQUAL_C(rval.GetError()->Code(), EBlackBoxErrorCode::BadData, ToString(rval.GetError()->Code()));
        }

        {
            auto rval = TBlackBoxApi{}.ParseTvm2UserTicket("{");
            UNIT_ASSERT(!rval.IsSuccess());
            UNIT_ASSERT_EQUAL_C(rval.GetError()->Code(), EBlackBoxErrorCode::BadData, ToString(rval.GetError()->Code()));
        }

        {
            auto rval = TBlackBoxApi{}.ParseFullInfo("{");
            UNIT_ASSERT(!rval.IsSuccess());
            UNIT_ASSERT_EQUAL_C(rval.GetError()->Code(), EBlackBoxErrorCode::BadData, ToString(rval.GetError()->Code()));
        }
    }

    Y_UNIT_TEST(ApiParseResponseSuccess) {
        static const TString blackBoxResponse = R"(
        {
           "dbfields" : {
              "userinfo.lastname.uid" : "lastname",
              "userinfo.firstname.uid" : "firstname"
           },
           "uid" : {
              "value" : "mock_uid"
           },
           "user_ticket" : "mock_user_ticket",
           "phones" : [
              {
                 "id" : "111",
                 "attributes" : {
                    "108" : "1",
                    "102" : "main_phone",
                    "107" : "default_phone"
                 }
              }
           ],
           "attributes" : {
              "1015" : "1"
           },
           "address-list" : [
              {
                 "address" : "root@yandex.ru"
              }
           ],
           "aliases" : {
              "13" : "root"
           },
           "billing_features" : {
              "basic-kinopoisk" : {
                 "in_trial": true,
                 "region": 225
              },
              "basic-plus" : {
                 "in_trial": true,
                 "region": 225
              },
              "basic-music" : {
                  "in_trial": true,
                  "region_id": 225
              }
           }
        })";

        {
            TString userTicket;
            auto status = TBlackBoxApi{}.ParseTvm2UserTicket(blackBoxResponse).MoveTo(userTicket);
            UNIT_ASSERT(!status.Defined());
            UNIT_ASSERT_EQUAL(userTicket, "mock_user_ticket");
        }

        {
            TString uid;
            auto status = TBlackBoxApi{}.ParseUid(blackBoxResponse).MoveTo(uid);
            UNIT_ASSERT(!status.Defined());
            UNIT_ASSERT_EQUAL(uid, "mock_uid");
        }

        {
            TBlackBoxFullUserInfoProto fullInfo;
            auto status = TBlackBoxApi{}.ParseFullInfo(blackBoxResponse).MoveTo(fullInfo);
            UNIT_ASSERT(!status.Defined());
            UNIT_ASSERT_VALUES_EQUAL(fullInfo.GetUserTicket(), "mock_user_ticket");
            UNIT_ASSERT_VALUES_EQUAL(fullInfo.GetUserInfo().GetUid(), "mock_uid");
            UNIT_ASSERT_VALUES_EQUAL(fullInfo.GetUserInfo().GetEmail(), "root@yandex.ru");
            UNIT_ASSERT_VALUES_EQUAL(fullInfo.GetUserInfo().GetFirstName(), "firstname");
            UNIT_ASSERT_VALUES_EQUAL(fullInfo.GetUserInfo().GetLastName(), "lastname");
            UNIT_ASSERT_VALUES_EQUAL(fullInfo.GetUserInfo().GetPhone(), "main_phone");
            UNIT_ASSERT_VALUES_EQUAL(fullInfo.GetUserInfo().GetHasYandexPlus(), true);
            UNIT_ASSERT_VALUES_EQUAL(fullInfo.GetUserInfo().GetIsStaff(), true);

            UNIT_ASSERT_EQUAL(fullInfo.GetUserInfo().GetSubscriptions().size(), 3);
            UNIT_ASSERT_VALUES_EQUAL(fullInfo.GetUserInfo().GetSubscriptions(0), "basic-kinopoisk");
            UNIT_ASSERT_VALUES_EQUAL(fullInfo.GetUserInfo().GetSubscriptions(1), "basic-music");
            UNIT_ASSERT_VALUES_EQUAL(fullInfo.GetUserInfo().GetSubscriptions(2), "basic-plus");

            UNIT_ASSERT_EQUAL(fullInfo.GetUserInfo().GetHasMusicSubscription(), true);
            UNIT_ASSERT_EQUAL(fullInfo.GetUserInfo().GetMusicSubscriptionRegionId(), 225);
        }
    }

    Y_UNIT_TEST(ApiParseResponsePhones) {
        struct TMockItem {
            TString Name;
            TString Json;
            TString Cannon;
        };
        static const TVector<TMockItem> items = {
            {
                "main_phone",
                R"({
                   "uid" : { "value" : "mock_uid" },
                   "phones" : [
                      {
                         "id" : "222",
                         "attributes" : { "102" : "some_phone" },
                      },
                      {
                         "id" : "111",
                         "attributes" : { "102" : "main_phone", "108" : "1" }
                      }
                   ]
                })",
                R"(UserInfo { Uid: "mock_uid" Phone: "main_phone" })",
            },
            {
                "default_phone",
                R"({
                   "phones" : [
                      {
                         "attributes" : { "102" : "some_phone" },
                         "id" : "222"
                      },
                      {
                         "id" : "333",
                         "attributes" : { "102" : "default_phone", "107" : "1" }
                      }
                   ],
                   "uid" : {
                      "value" : "mock_uid"
                   }
                })",
                R"(UserInfo { Uid: "mock_uid" Phone: "default_phone" })",
            },
            {
                "just first found phone (some_phone1)",
                R"({
                   "uid" : { "value" : "mock_uid" },
                   "phones" : [
                      {
                         "id" : "222",
                         "attributes" : {
                            "102" : "some_phone1"
                         },
                      },
                      {
                         "id" : "333",
                         "attributes" : {
                            "102" : "some_phone2"
                         },
                      }
                   ]
                })",
                R"(UserInfo { Uid: "mock_uid" Phone: "some_phone1" })",
            }
        };

        for (const auto& item : items) {
            TBlackBoxFullUserInfoProto fullInfo;
            auto status = TBlackBoxApi{}.ParseFullInfo(item.Json).MoveTo(fullInfo);
            UNIT_ASSERT_C(!status.Defined(), item.Name);
            UNIT_ASSERT_EQUAL_C(fullInfo.ShortUtf8DebugString(), item.Cannon, item.Name);
        }
    }
}

} // namespace
