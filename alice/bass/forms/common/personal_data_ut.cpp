#include "personal_data.h"

#include <alice/bass/ut/helpers.h>

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/vector.h>

using namespace NBASS;
using namespace NTestingHelpers;

using THelper = TPersonalDataHelper;
using TKeyValue = THelper::TKeyValue;
using EUserSpecificKey = TPersonalDataHelper::EUserSpecificKey;

namespace {
const NSc::TValue EMPTY = NSc::TValue::FromJson(R"({"items": []})");

const NSc::TValue SIMPLE = NSc::TValue::FromJson(R"({"items": [
  {
    "method": "PUT",
    "relative_url": "/v1/personality/profile/alisa/kv/user_name",
    "body": "{\"value\":\"john doe\"}"
  },
  {
    "method": "PUT",
    "relative_url": "/v1/personality/profile/alisa/kv/gender",
    "body": "{\"value\":\"male\"}"
  },
  {
    "method": "PUT",
    "relative_url": "/v1/personality/profile/alisa/kv/user_name",
    "body": "{\"value\":\"tricky\\\"name\"}"
  },
  {
    "method": "PUT",
    "relative_url": "/v1/personality/profile/alisa/kv/gender",
    "body": "{\"value\":\"\"}"
  }
]})");

// Copy-pasted from doc: https://wiki.yandex-team.ru/disk/mpfs/platformapibatchrequests/
const TString CORRECT_BATCH_RESPONSE = R"({
    "items": [
        {
            "body": "{\"items\":[],\"total\":0,\"limit\":20,\"offset\":0}",
            "code": 200,
            "headers": {
                "Access-Control-Allow-Headers": "Accept-Language, Accept, X-HTTP-Method, X-Requested-With, Content-Type, Authorization",
                "Access-Control-Allow-Methods": "GET, OPTIONS",
                "Access-Control-Allow-Origin": "*",
                "Content-Type": "application/json; charset=utf-8"
            }
        },
        {
            "body": "{\"items\":[{\"slug\":\"home\",\"geo_id\":123},{\"slug\":\"frequently-visited\",\"geo_id\":129}],\"total\":2,\"limit\":20,\"flags\":[\"edge\"],\"offset\":0}",
            "code": 200,
            "headers": {
                "Access-Control-Allow-Headers": "Accept-Language, Accept, X-HTTP-Method, X-Requested-With, Content-Type, Authorization",
                "Access-Control-Allow-Methods": "PUT, GET, OPTIONS",
                "Access-Control-Allow-Origin": "*",
                "Content-Type": "application/json; charset=utf-8"
            }
        },
        {
            "body": "{\"items\":[{\"data_source\":\"teist\",\"flight_number\":\"SU2030\",\"id\":\"hrew1423755744325cot284676689\",\"departure\":{\"city_name\":\"Moscow\",\"time\":\"2015-02-18T13:59:08.773000+00:00\"}}],\"total\":1,\"limit\":20,\"offset\":0}",
            "code": 200,
            "headers": {
                "Access-Control-Allow-Headers": "Accept-Language, Accept, X-HTTP-Method, X-Requested-With, Content-Type, Authorization",
                "Access-Control-Allow-Methods": "POST, GET, OPTIONS",
                "Access-Control-Allow-Origin": "*",
                "Content-Type": "application/json; charset=utf-8"
            }
        }
    ]
})";

const TString MALFORMED_BATCH_RESPONSE = R"({
    "results": [
        {
            "code": 200
        },
        {
            "code": 200
        },
        {
            "code": 200
        }
    ]
})";

const TString MALFORMED_JSON_BATCH_RESPONSE = R"({
    "results": [
        {
            "code": 200
        },
        {
            "code": 200
        },
        {
            "code": 200
        }
})";

const TString ERROR_BATCH_RESPONSE = R"({
    "items": [
        {
            "code": 200
        },
        {
            "code": 404
        },
        {
            "code": 200
        }
    ]
})";

NHttpFetcher::TResponse::TRef ResponseWithData(const TString& data) {
    return new NHttpFetcher::TResponse(HTTP_OK, data, TDuration(), "", THttpHeaders());
}

TContext::TPtr MakeMockContext() {
    return MakeContext(NSc::TValue::FromJson(R"({"meta": {"epoch": 1526559187, "tz": "Europe/Moscow"}})"));
}

} // namespace

Y_UNIT_TEST_SUITE(TPersonalDataHelperUnitTest) {
    Y_UNIT_TEST(DataSyncBatchRequestSmoke) {
        const NSc::TValue actual = THelper(*MakeMockContext()).PrepareDataSyncBatchRequestContent(TVector<THelper::TKeyValue>{});
        UNIT_ASSERT(EqualJson(EMPTY, actual));
    }

    Y_UNIT_TEST(DataSyncBatchRequestSimple) {
        TVector<TKeyValue> kvs;
        kvs.emplace_back(EUserSpecificKey::UserName, "john doe"sv);
        kvs.emplace_back(EUserSpecificKey::Gender, "male"sv);
        kvs.emplace_back(EUserSpecificKey::UserName, "tricky\"name"sv);
        kvs.emplace_back(EUserSpecificKey::Gender, ""sv);

        const NSc::TValue actual = THelper(*MakeMockContext()).PrepareDataSyncBatchRequestContent(kvs);
        UNIT_ASSERT(EqualJson(SIMPLE, actual));
    }

    Y_UNIT_TEST(VerifyBatchResponseCorrect) {
        int callCount = 0;
        auto fn = [&callCount](TStringBuilder&) { ++callCount; };
        UNIT_ASSERT(!THelper::VerifyBatchResponse(ResponseWithData(CORRECT_BATCH_RESPONSE), fn));
        UNIT_ASSERT_EQUAL(callCount, 0);
    }

    Y_UNIT_TEST(VerifyBatchResponseMalformed) {
        int callCount = 0;
        auto fn = [&callCount](TStringBuilder&) { ++callCount; };
        UNIT_ASSERT(THelper::VerifyBatchResponse(ResponseWithData(MALFORMED_BATCH_RESPONSE), fn));
        UNIT_ASSERT_EQUAL(callCount, 1);
    }

    Y_UNIT_TEST(VerifyBatchResponseMalformedJson) {
        int callCount = 0;
        auto fn = [&callCount](TStringBuilder&) { ++callCount; };
        UNIT_ASSERT(THelper::VerifyBatchResponse(ResponseWithData(MALFORMED_JSON_BATCH_RESPONSE), fn));
        UNIT_ASSERT_EQUAL(callCount, 1);
    }

    Y_UNIT_TEST(VerifyBatchResponseError) {
        int callCount = 0;
        auto fn = [&callCount](TStringBuilder&) { ++callCount; };
        UNIT_ASSERT(THelper::VerifyBatchResponse(ResponseWithData(ERROR_BATCH_RESPONSE), fn));
        UNIT_ASSERT_EQUAL(callCount, 1);
    }

    Y_UNIT_TEST(UidFromDataSource) {
        auto ctxPtr = NTestingHelpers::CreateVideoContextWithAgeRestriction(
            EContentRestrictionLevel::Without, [](const NSc::TValue& context) {
                auto newCtx = context;
                newCtx["data_sources"]["2"]["user_info"]["uid"] = "12345";
                return MakeContext(newCtx);
            });

        THelper helper{*ctxPtr};
        TString uid;
        UNIT_ASSERT(helper.GetUid(uid));
        UNIT_ASSERT_STRINGS_EQUAL("12345", uid);
    }

    Y_UNIT_TEST(UidFromMeta) {
        auto ctxPtr = NTestingHelpers::MakeContext(NSc::TValue(TRequestJson{}.SetUID(12345)));
        THelper helper{*ctxPtr};
        TString uid;
        UNIT_ASSERT(helper.GetUid(uid));
        UNIT_ASSERT_STRINGS_EQUAL("12345", uid);
    }

    Y_UNIT_TEST(UserTicketFromContext) {
        auto ctxPtr = NTestingHelpers::MakeContext(NSc::TValue(TRequestJson{}), /* shouldValidateContext= */ true,
                                                   /* userTicket= */ "1234");
        THelper helper{*ctxPtr};
        TString userTicket;
        UNIT_ASSERT(helper.GetTVM2UserTicket(userTicket));
        UNIT_ASSERT_STRINGS_EQUAL("1234", userTicket);
    }
}
