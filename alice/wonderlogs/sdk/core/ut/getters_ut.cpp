#include <alice/wonderlogs/sdk/core/getters.h>

#include <google/protobuf/text_format.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/maybe.h>

#include <library/cpp/json/json_reader.h>

using namespace NAlice::NMegamind;
using namespace NAlice::NWonderSdk;
using namespace google::protobuf;

namespace {

const TString MEGAMIND_ANALYTICS_INFO_HOLLYWOOD_MUSIC_PROTO = R"(
AnalyticsInfo {
    key: "HollywoodMusic"
    value {
        ScenarioAnalyticsInfo {
            Intent: "keklol",
            ProductScenarioName: "lolkek1337"
            Events {
                MusicEvent {
                    AnswerType: Track
                }
            }
            Events {
                MusicEvent {
                    Id: "genre:hardbass"
                    AnswerType: Filters
                }
            }
            Objects {
                FirstTrack {
                    Genre: "phonk"
                }
            }
        }
    }
}
WinnerScenario {
    Name: "HollywoodMusic"
}
)";

constexpr TStringBuf CALLBACK_ARGS_WITH_SUGGEST_FORM_UPDATE_NAME_BLOCK_JSON = R"(
{
    "form_update": {
        "name": "flex69"
    }
}
)";

constexpr TStringBuf CALLBACK_ARGS_WITHOUT_SUGGEST_FORM_UPDATE_NAME_BLOCK_JSON = R"(
{
    "form_update": {
    }
}
)";

constexpr TStringBuf ACTION_JSON = R"({
    "log_id":"skill_recommendation__get_greetings__editorial#__show_traffic",
    "url":"dialog-action://?directives=%5B%0A%20%20%20%20%7B%22type%22%3A%22client_action%22%2C%22sub_name%22%3A%22skill_recommendation__get_greetings__editorial%23__show_traffic%22%2C%22name%22%3A%22type%22%2C%22payload%22%3A%7B%22text%22%3A%22%5Cu041a%5Cu0430%5Cu043a%5Cu0438%5Cu0435%20%5Cu0441%5Cu0435%5Cu0439%5Cu0447%5Cu0430%5Cu0441%20%5Cu043f%5Cu0440%5Cu043e%5Cu0431%5Cu043a%5Cu0438%3F%22%7D%7D%2C%7B%22type%22%3A%22server_action%22%2C%22name%22%3A%22external_source_action%22%2C%22ignore_answer%22%3Atrue%2C%22payload%22%3A%7B%22utm_campaign%22%3A%22%22%2C%22utm_term%22%3A%22%22%2C%22utm_content%22%3A%22textlink%22%2C%22utm_source%22%3A%22Yandex_Alisa%22%2C%22utm_medium%22%3A%22get_greetings%22%2C%22request_id%22%3A%2280f47636-10df-4ae9-85ba-61d557361675%22%7D%7D%2C%7B%22type%22%3A%22server_action%22%2C%22name%22%3A%22on_card_action%22%2C%22ignore_answer%22%3Atrue%2C%22payload%22%3A%7B%22item_number%22%3A1%2C%22request_id%22%3A%2280f47636-10df-4ae9-85ba-61d557361675%22%2C%22intent_name%22%3A%22personal_assistant.scenarios.skill_recommendation%22%2C%22card_id%22%3A%22skill_recommendation%22%2C%22case_name%22%3A%22skill_recommendation__get_greetings__editorial%23__show_traffic%22%7D%7D%0A%20%20%20%20%5D"
})";

Y_UNIT_TEST_SUITE(Getters) {
    Y_UNIT_TEST(GetFiltersGenre) {
        TMegamindAnalyticsInfo megamindAnalyticsInfo;
        UNIT_ASSERT(
            TextFormat::ParseFromString(MEGAMIND_ANALYTICS_INFO_HOLLYWOOD_MUSIC_PROTO, &megamindAnalyticsInfo));
        const auto actual = GetFiltersGenre(megamindAnalyticsInfo);
        UNIT_ASSERT(actual);
        UNIT_ASSERT_VALUES_EQUAL("hardbass", *actual);
    }

    Y_UNIT_TEST(GetPath) {
        TMegamindAnalyticsInfo megamindAnalyticsInfo;
        UNIT_ASSERT(
            TextFormat::ParseFromString(MEGAMIND_ANALYTICS_INFO_HOLLYWOOD_MUSIC_PROTO, &megamindAnalyticsInfo));
        auto res = GetPath(megamindAnalyticsInfo,
                           NJson::ReadJsonFastTree(CALLBACK_ARGS_WITH_SUGGEST_FORM_UPDATE_NAME_BLOCK_JSON));

        UNIT_ASSERT(res);
        UNIT_ASSERT_VALUES_EQUAL("HollywoodMusic", *res);
    }

    Y_UNIT_TEST(RestoreFormNameWithFormUpdateName) {
        TMegamindAnalyticsInfo megamindAnalyticsInfo;
        UNIT_ASSERT(
            TextFormat::ParseFromString(MEGAMIND_ANALYTICS_INFO_HOLLYWOOD_MUSIC_PROTO, &megamindAnalyticsInfo));
        auto res = NImpl::RestoreFormName(
            megamindAnalyticsInfo, NJson::ReadJsonFastTree(CALLBACK_ARGS_WITH_SUGGEST_FORM_UPDATE_NAME_BLOCK_JSON));

        UNIT_ASSERT(res);
        UNIT_ASSERT_VALUES_EQUAL("flex69", *res);
    }

    Y_UNIT_TEST(RestoreFormNameWithoutFormUpdateName) {
        TMegamindAnalyticsInfo megamindAnalyticsInfo;
        UNIT_ASSERT(
            TextFormat::ParseFromString(MEGAMIND_ANALYTICS_INFO_HOLLYWOOD_MUSIC_PROTO, &megamindAnalyticsInfo));
        auto res = NImpl::RestoreFormName(
            megamindAnalyticsInfo, NJson::ReadJsonFastTree(CALLBACK_ARGS_WITHOUT_SUGGEST_FORM_UPDATE_NAME_BLOCK_JSON));

        UNIT_ASSERT(res);
        UNIT_ASSERT_VALUES_EQUAL("HollywoodMusic", *res);
    }

    Y_UNIT_TEST(ParseAction) {
        const auto action = NJson::ReadJsonFastTree(ACTION_JSON);

        NImpl::TAction expected;
        expected.ActionName = "skill_recommendation__get_greetings__editorial#__show_traffic";
        expected.IntentName = "personal_assistant.scenarios.skill_recommendation";
        expected.CardId = "skill_recommendation";
        const auto actual = NImpl::ParseAction(action, /* onlyLogId= */ false);
        UNIT_ASSERT_VALUES_EQUAL(expected.ActionName, actual.ActionName);
        UNIT_ASSERT_VALUES_EQUAL(expected.IntentName, actual.IntentName);
        UNIT_ASSERT_VALUES_EQUAL(expected.CardId, actual.CardId);
    }
}

} // namespace
