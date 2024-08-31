#include "logs_util.h"

#include <library/cpp/json/json_reader.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/vector.h>

using namespace NAlice::NMegamind::NLogsUtil;

namespace {

Y_UNIT_TEST_SUITE(LogsUtil) {
    Y_UNIT_TEST(Obfuscate) {
        struct TTest {
            TStringBuf Comment;
            TStringBuf JsonRaw;
            TStringBuf Canon;
        };

        TVector<TTest> tests = {
            {
                "bad json",
                R"({aaa})",
                R"({aaa})",
            },
            {
                "no token",
                R"({"super": "duper"})",
                R"({"super": "duper"})"
            },
            {
                "token obfuscated",
                R"({"request":{"additional_options":{"oauth_token":"abcdef"}}})",
                R"({"request":{"additional_options":{"oauth_token":"**OBFUSCATED**"}}})"
            },
            {
                "token is empty dict",
                R"({"request":{"additional_options":{"oauth_token":{}}}})",
                R"({"request":{"additional_options":{"oauth_token":"**OBFUSCATED**"}}})"
            },
            {
                "token is number",
                R"({"request":{"additional_options":{"oauth_token":1}}})",
                R"({"request":{"additional_options":{"oauth_token":"**OBFUSCATED**"}}})"
            },
            {
                "token is array",
                R"({"request":{"additional_options":{"oauth_token":["1"]}}})",
                R"({"request":{"additional_options":{"oauth_token":"**OBFUSCATED**"}}})"
            },
            {
                "cookies",
                R"({"request":{"additional_options":{"bass_options":{"cookies":["1"]}}}})",
                R"({"request":{"additional_options":{"bass_options":{"cookies":"**OBFUSCATED**"}}}})"
            },
            {
                "contacts",
                R"({"contacts":{"data":{"contacts":[{"contact_id":123}],"phones":[{"_id":123,"phone":"+71234567890"}]}}})",
                R"({"contacts":{"data":"**OBFUSCATED**"}})",
            },
            {
                "ignore_answer_true",
                R"({"request":{"event":{"ignore_answer":true}},"session":"abc"})",
                R"({"request":{"event":{"ignore_answer":true}},"session":"**OBFUSCATED**"})"
            },
            {
                "ignore_answer_false",
                R"({"request":{"event":{"ignore_answer":false}},"session":"abc"})",
                R"({"request":{"event":{"ignore_answer":false}},"session":"abc"})"
            },
        };

        for (const auto& test : tests) {
            TString toCheck;
            NJson::TJsonValue json;
            try {
                json = NJson::ReadJsonFastTree(test.JsonRaw);
            } catch (...) {
            }
            ObfuscateBody(
                test.JsonRaw, json, [&toCheck](TStringBuf body) { toCheck = body; }, BASE_PATHS_TO_BE_OBFUSCATED_IN_REQUEST);
            UNIT_ASSERT_VALUES_EQUAL_C(test.Canon, toCheck, test.Comment);
        }
    }
}

}  // namespace
