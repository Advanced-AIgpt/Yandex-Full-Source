#include "ifs_map.h"

#include <alice/bass/forms/context/context.h>

#include <alice/bass/ut/helpers.h>

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>


using namespace NBASS;

namespace {

// R"-( because string contains )"
const NSc::TValue SIMPLE_REQUEST = NSc::TValue::FromJson(TStringBuf(R"-(
{
  "meta": {
    "epoch": 1504271099,
    "tz": "UTC",
    "uuid": "00000000-0000-0000-0000-000000000000",
    "utterance": "hello",
    "uid": 4007095345,
    "client_id" : "ru.yandex.searchplugin.dev/7.10 (none none; android 7.1.2)",
    "user_agent" : "Mozilla/5.0 (Linux; Android 7.1.2; Redmi Note 4 Build/NJH47F; wv) AppleWebKit/537.36 (KHTML, like Gecko) Version/4.0 Chrome/63.0.3239.111 Mobile Safari/537.36 YandexSearch/7.10",
  },
  "form": {
    "slots": [],
    "name": "personal_assistant.scenarios.external_skill"
  }
}
)-"));

TContext::TPtr CreateContext(TStringBuf clientId) {
    NSc::TValue json = SIMPLE_REQUEST;
    json["meta"]["client_id"].SetString(clientId);
    return NTestingHelpers::MakeContext(json);
}

struct TCheckItem {
    TStringBuf ClientId;
    TStringBuf CanonJson;
};

} // namespace

Y_UNIT_TEST_SUITE(InterfacesMapTestSuite) {
    Y_UNIT_TEST(InterfacesMap) {
        const TVector<TCheckItem> checkItemsList = {
            { // quasar
                "ru.yandex.quasar.services/1.0 (yandex station; android 6.0.1)",
                R"({ "account_linking": {}, "payments": {} })"
            },
            { // watch
                "ru.yandex.iosdk.elariwatch/1.0 (kidphone3g kidphone3g; android 4.4.2)",
                R"({})"
            },
            { // naviagator
                "ru.yandex.yandexnavi/3.31 (zuk zuk z1; android 7.1.2)",
                R"({})"
            },
            { // auto
                "yandex.auto/1.2.3 (none none; android 4.4)",
                R"({})"
            },
            { // everything else
                "ru.yandex.searchplugin.dev/7.80 (Xiaomi Redmi Note 4; android 7.1.2)",
                R"({ "account_linking": {}, "payments": {}, "screen": {} })"
            },
        };

        for (const TCheckItem& ci : checkItemsList) {
            TContext::TPtr ctx = CreateContext(ci.ClientId);
            UNIT_ASSERT(ctx);
            UNIT_ASSERT_VALUES_EQUAL_C(NSc::TValue::FromJson(ci.CanonJson), NExternalSkill::CreateHookInterfaces(*ctx), ci.ClientId);
        }
    }
}
