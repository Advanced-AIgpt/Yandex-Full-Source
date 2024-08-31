#include "blackbox.h"

#include <alice/megamind/library/apphost_request/item_names.h>
#include <alice/megamind/library/apphost_request/util.h>
#include <alice/megamind/library/testing/apphost_helpers.h>
#include <alice/megamind/library/testing/components.h>

#include <alice/library/blackbox/blackbox.h>
#include <alice/library/network/headers.h>

#include <apphost/lib/proto_answers/http.pb.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/algorithm.h>
#include <util/string/builder.h>

namespace {

using namespace NAlice;
using namespace NAlice::NMegamind;
using namespace NAlice::NMegamind::NTesting;
using namespace testing;

const TString TICKET = "my_super_duper_ticket";
const TString OAUTH_TOKEN = "oauth my_super_secret_token";
const TString SAMPLE_TEXT = "sample text";

NAppHostHttp::THttpRequest SetupBlackBox(TTestAppHostCtx& ahCtx, const TString* clientIp, const TString* authToken) {
    TStringBuilder msg;
    msg << "client ip: " << (clientIp ? *clientIp : "(none)")
        << "auth token: " << (authToken ? *authToken : "(none)");

    TTestComponents<TTestClientComponent> components;
    auto& user = components.Get<TTestClientComponent>();

    EXPECT_CALL(Const(user), ClientIp()).WillRepeatedly(Return(clientIp));
    EXPECT_CALL(Const(user), AuthToken()).WillRepeatedly(Return(authToken));

    {
        TStatus status = AppHostBlackBoxSetup(ahCtx, components);
        UNIT_ASSERT_C(!status.Defined(), msg);
    }

    if (!authToken) {
        return {};
    }

    NAppHostHttp::THttpRequest proto;
    auto status = ahCtx.TestCtx().GetOnlyProtobufItem(AH_ITEM_BLACKBOX_HTTP_REQUEST_NAME).Fill(&proto);
    UNIT_ASSERT_C(!status == !clientIp, msg);

    return proto;
}

Y_UNIT_TEST_SUITE_F(AppHostMegamindBlackBox, TAppHostFixture) {
    Y_UNIT_TEST(SetupSuccessWithAuthToken) {
        auto ahCtx = CreateAppHostContext();
        const TString clientIp{"127.0.0.1"};
        auto proto = SetupBlackBox(ahCtx, &clientIp, &OAUTH_TOKEN);
        UNIT_ASSERT_VALUES_EQUAL(proto.GetPath(), TStringBuilder{} <<
                                 "?aliases=13&attributes=1015%2C8&dbfields=userinfo.firstname.uid%2Cuserinfo.lastname"
                                 ".uid&emails=getdefault&format=json&get_billing_features=all&get_login_id=yes&get_user_ticket=yes"
                                 "&getphones=bound&method=oauth&phone_attributes=107%2C102%2C108&userip=" << clientIp);
        UNIT_ASSERT(proto.GetScheme() == NAppHostHttp::THttpRequest_EScheme::THttpRequest_EScheme_Https);
        const THashMap<TStringBuf, TString> headers = {
            { NNetwork::HEADER_AUTHORIZATION, OAUTH_TOKEN },
        };

        auto findHeader = [&headers](const auto& h) {
            const auto* value = headers.FindPtr(h.GetName());
            return value ? *value == h.GetValue() : 0;
        };
        UNIT_ASSERT_VALUES_EQUAL(headers.size(), CountIf(proto.GetHeaders(), findHeader));
        UNIT_ASSERT_VALUES_EQUAL(proto.HeadersSize(), headers.size());
    }

    Y_UNIT_TEST(SetupFailWithoutAuthToken) {
        auto ahCtx = CreateAppHostContext();
        const TString clientIp{"127.0.0.1"};
        auto proto = SetupBlackBox(ahCtx, &clientIp, nullptr);
        UNIT_ASSERT(proto.GetPath().Empty());
    }

    Y_UNIT_TEST(SetupFail) {
        auto ahCtx = CreateAppHostContext();
        SetupBlackBox(ahCtx, nullptr, nullptr);
    }

    Y_UNIT_TEST(PostSetupNoResponse) {
        auto ahCtx = CreateAppHostContext();
        TBlackBoxFullUserInfoProto proto;
        TStatus status = AppHostBlackBoxPostSetup(ahCtx, proto);
        UNIT_ASSERT(status.Defined());
        UNIT_ASSERT_VALUES_EQUAL(status->Type, TError::EType::Logic);
    }

    Y_UNIT_TEST(PostSetupSuccess) {
        static const TString json = R"(
{
   "address-list" : [
      {
         "address" : "root@yandex.ru",
         "born-date" : "2008-04-22 16:16:46",
         "default" : true,
         "native" : true,
         "rpop" : false,
         "silent" : false,
         "unsafe" : false,
         "validated" : true
      }
   ],
   "aliases" : {
      "13" : "bobik"
   },
   "attributes" : {
      "1015" : "1"
   },
   "connection_id" : "t:13763464576",
   "dbfields" : {
      "userinfo.firstname.uid" : "Vasily",
      "userinfo.lastname.uid" : "Pupkin"
   },
   "error" : "OK",
   "have_hint" : true,
   "have_password" : true,
   "karma" : {
      "value" : 0
   },
   "karma_status" : {
      "value" : 6000
   },
   "login" : "yabobik",
   "oauth" : {
      "client_ctime" : "2015-05-20 16:08:59",
      "client_homepage" : "",
      "client_icon" : "https://avatars.mds.yandex.net/get-oauth/39247/f8cab64f154b4c8e96f92dac8becfcaa-1/normal",
      "client_id" : "super_duper_client_id",
      "client_is_yandex" : true,
      "client_name" : "client_name",
      "ctime" : "2019-04-10 13:58:41",
      "device_id" : "my_super_duper_device",
      "device_name" : "Super Device",
      "expire_time" : "2021-02-25 05:08:12",
      "is_ttl_refreshable" : true,
      "issue_time" : "2019-08-27 13:50:56",
      "meta" : "",
      "scope" : "mobile:all mobmail:all music:read yadisk:all yadisk:browser_sync yataxi:pay yataxi:read yataxi:write cloud_api.data:user_datacloud_api.data:app_data yamb:all quasar:all quasar:pay passport:bind_email quasar:glagol",
      "token_id" : "87654321",
      "uid" : "12345678",
      "xtoken_id" : "12345678"
   },
   "phones" : [
      {
         "attributes" : {
            "102" : "+70000000070",
            "107" : "1"
         },
         "id" : "94457890"
      }
   ],
   "status" : {
      "id" : 0,
      "value" : "VALID"
   },
   "uid" : {
      "hosted" : false,
      "lite" : false,
      "value" : "01234567"
   },
   "user_ticket" : "super_duper_ticket"
})";

        auto ahCtx = CreateAppHostContext();

        NAppHostHttp::THttpResponse responseProto;
        responseProto.SetStatusCode(HTTP_OK);
        responseProto.SetContent(json);
        ahCtx.TestCtx().AddProtobufItem(responseProto, AH_ITEM_BLACKBOX_HTTP_RESPONSE_NAME, NAppHost::EContextItemKind::Input);
        {
            TBlackBoxFullUserInfoProto proto;
            auto status = AppHostBlackBoxPostSetup(ahCtx, proto);
            UNIT_ASSERT(!status.Defined());
            UNIT_ASSERT(proto.IsInitialized());
        }

        {
            TBlackBoxApi::TFullUserInfo fullInfo;
            UNIT_ASSERT(ahCtx.TestCtx().GetOnlyProtobufItem(AH_ITEM_BLACKBOX).Fill(&fullInfo));

            UNIT_ASSERT_VALUES_EQUAL(fullInfo.GetUserTicket(), "super_duper_ticket");
            UNIT_ASSERT_VALUES_EQUAL(fullInfo.GetUserInfo().GetUid(), "01234567");
            UNIT_ASSERT_VALUES_EQUAL(fullInfo.GetUserInfo().GetEmail(), "root@yandex.ru");
            UNIT_ASSERT_VALUES_EQUAL(fullInfo.GetUserInfo().GetFirstName(), "Vasily");
            UNIT_ASSERT_VALUES_EQUAL(fullInfo.GetUserInfo().GetLastName(), "Pupkin");
            UNIT_ASSERT_VALUES_EQUAL(fullInfo.GetUserInfo().GetPhone(), "+70000000070");
            UNIT_ASSERT_VALUES_EQUAL(fullInfo.GetUserInfo().GetHasYandexPlus(), true);
            UNIT_ASSERT_VALUES_EQUAL(fullInfo.GetUserInfo().GetIsStaff(), true);
        }
    }
}

} // namespace
