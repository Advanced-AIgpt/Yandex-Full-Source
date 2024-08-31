#include <alice/library/json/json.h>
#include <alice/library/unittest/message_diff.h>
#include <alice/megamind/library/analytics/analytics_info.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NMegamind {

namespace {

const TString SEARCH_TEXT_KEY = "search_text";
const TString SEARCH_TEXT_VALUE = "hello";

TSemanticFrame CreateSemanticFrame() {
    TSemanticFrame frame;

    auto& slot = *frame.AddSlots();
    slot.SetName(SEARCH_TEXT_KEY);
    slot.MutableTypedValue()->SetType(SEARCH_TEXT_KEY);
    slot.MutableTypedValue()->SetString(SEARCH_TEXT_VALUE);

    return frame;
}

TSemanticFrame CreateSemanticFrame(const TString& frameName) {
    auto frame = CreateSemanticFrame();
    frame.SetName(frameName);

    return frame;
}

constexpr TStringBuf ANALYTICS_INFO_JSON = TStringBuf(R"(
{
  "scenario_analytics_info": {
    "actions": [
      {
        "name": "turn_on",
        "human_readable": "turn on",
        "id": "iot.turn.on"
      }
    ],
    "objects": [
      {
        "name": "light_bulb",
        "human_readable": "light bulb",
        "id": "iot.light_bulb"
      }
    ],
    "events": [
      {
        "timestamp": "1566819102000000",
        "request_source_event": {
          "cgi": {
            "rnd": "1",
            "tvm": "tvm_value"
          },
          "source": "http://source_request_1"
        }
      },
      {
        "timestamp": "1566819102000000",
        "request_source_event": {
          "cgi": {
            "rnd": "0",
            "tvm": "tvm_value_2"
          },
          "source": "http://source_request_2"
        }
      }
    ]
  }
}
)");

constexpr TStringBuf ANALYTICS_INFO_2_JSON = TStringBuf(R"(
{
  "scenario_analytics_info": {
    "actions": [
      {
        "name": "turn_on 2",
        "human_readable": "turn on 2",
        "id": "iot.turn.on.2"
      }
    ],
    "objects": [
      {
        "name": "light_bulb_2",
        "human_readable": "light bulb 2",
        "id": "iot.light_bulb.2"
      }
    ]
  }
}
)");

constexpr TStringBuf USER_INFO_JSON = TStringBuf(R"(
{
  "scenario_user_info": {
    "properties": [{
        "id": "id",
        "name": "name",
        "human_readable": "description",
        "profile": {
            "params": {
                "profile.key.1": {
                    "value": "profile.value.1",
                    "human_readable": "profile.description.1"
                },
                "profile.key.2": {
                    "value": "profile.value.2",
                    "human_readable": "profile.description.2"
                }
            }
        }
    }]
  }
}
)");

constexpr TStringBuf USER_INFO_2_JSON = TStringBuf(R"(
{
  "scenario_user_info": {
    "properties": [{
        "id": "id.2",
        "name": "name_2",
        "human_readable": "description 2",
        "profile": {
            "params": {
                "profile.key.3": {
                    "value": "profile.value.3",
                    "human_readable": "profile.description.3"
                },
                "profile.key.4": {
                    "value": "profile.value.4",
                    "human_readable": "profile.description.4"
                }
            }
        }
    }]
  }
}
)");

constexpr TStringBuf ANALYTICS_INFO_SEMANTIC_FRAME_JSON = TStringBuf(R"(
{
  "semantic_frame": {
    "name": "intent_from_frame",
    "slots": [{
      "name": "search_text",
      "typed_value": {
        "type": "search_text",
        "string": "hello"
      }
    }]
  }
}
)");

constexpr TStringBuf ANALYTICS_INFO_SEMANTIC_FRAME_WITHOUT_INTENT_JSON = TStringBuf(R"(
{
  "semantic_frame": {
    "slots": [{
      "name": "search_text",
      "typed_value": {
        "type": "search_text",
        "string": "hello"
      }
    }]
  }
}
)");

constexpr TStringBuf ANALYTICS_INFO_WITH_SCENARIO_AND_SEMANTIC_FRAME_JSON = TStringBuf(R"(
{
  "scenario_analytics_info": {
    "intent": "intent_scenario_name"
  },
  "semantic_frame": {
    "name": "intent_from_frame",
    "slots": [{
      "name": "search_text",
      "typed_value": {
        "type": "search_text",
        "string": "hello"
      }
    }]
  }
}
)");

constexpr TStringBuf ANALYTICS_INFO_WITH_SCENARIO_AND_SEMANTIC_FRAME_WITHOUT_INTENT_JSON = TStringBuf(R"(
{
  "scenario_analytics_info": {
    "intent": "intent_scenario_name"
  },
  "semantic_frame": {
    "slots": [{
      "name": "search_text",
      "typed_value": {
        "type": "search_text",
        "string": "hello"
      }
    }]
  }
}
)");

} // namespace

Y_UNIT_TEST_SUITE(AnalyticsInfoBuilder) {
    Y_UNIT_TEST(CheckFillingAnalyticsInfoBuilder) {
        TAnalyticsInfoBuilder builder;
        builder.CreateScenarioAnalyticsInfoBuilder()
            ->AddRequestSourceEvent(TInstant::ParseIso8601("2019-08-26T11:31:42"), "http://source_request_1")
                ->AddCgiParam("tvm", "tvm_value")
                .AddCgiParam("rnd", "1")
                .Build()
            .AddRequestSourceEvent(TInstant::ParseIso8601("2019-08-26T11:31:42"), "http://source_request_2")
                ->AddCgiParam("tvm", "tvm_value_2")
                .AddCgiParam("rnd", "0")
                .Build()
            .AddObject("iot.light_bulb", "light_bulb", "light bulb")
            .AddAction("iot.turn.on", "turn_on", "turn on");

        const auto json = JsonFromString(ANALYTICS_INFO_JSON);
        TAnalyticsInfo expected;
        UNIT_ASSERT(JsonToProto(json, expected).ok());

        const auto analyticsInfo = builder.Build();
        UNIT_ASSERT_MESSAGES_EQUAL(expected, analyticsInfo);
    }

    Y_UNIT_TEST(CheckFillingAnalyticsInfoBuilderOnSecondTime) {
        TAnalyticsInfoBuilder builder;
        builder.CreateScenarioAnalyticsInfoBuilder()
            ->AddRequestSourceEvent(TInstant::ParseIso8601("2019-08-26T11:31:42"), "http://source_request_1")
                ->AddCgiParam("tvm", "tvm_value")
                .AddCgiParam("rnd", "1")
                .Build()
            .AddRequestSourceEvent(TInstant::ParseIso8601("2019-08-26T11:31:42"), "http://source_request_2")
                ->AddCgiParam("tvm", "tvm_value_2")
                .AddCgiParam("rnd", "0")
                .Build()
            .AddObject("iot.light_bulb", "light_bulb", "light bulb")
            .AddAction("iot.turn.on", "turn_on", "turn on");

        builder.Build();

        builder.CreateScenarioAnalyticsInfoBuilder()
            ->AddObject("iot.light_bulb.2", "light_bulb_2", "light bulb 2")
            .AddAction("iot.turn.on.2", "turn_on 2", "turn on 2");

        const auto json = JsonFromString(ANALYTICS_INFO_2_JSON);
        TAnalyticsInfo expected;
        UNIT_ASSERT(JsonToProto(json, expected).ok());

        const auto analyticsInfo = builder.Build();
        UNIT_ASSERT_MESSAGES_EQUAL(expected, analyticsInfo);
    }

    Y_UNIT_TEST(SetVersionAnalyticsInfoBuilder) {
        TAnalyticsInfoBuilder builder;
        builder.SetVersion("20.20");

        const auto analyticsInfo = builder.Build();
        UNIT_ASSERT_EQUAL("20.20", analyticsInfo.GetVersion());
    }

    Y_UNIT_TEST(SetEmptyVersionAnalyticsInfoBuilder) {
        TAnalyticsInfoBuilder builder;
        builder.SetVersion("");

        const auto analyticsInfo = builder.Build();
        UNIT_ASSERT_EQUAL("", analyticsInfo.GetVersion());
    }

    Y_UNIT_TEST(SetEmptyVersionInCreatedAnalyticsInfoBuilder) {
        TAnalyticsInfoBuilder builder;
        builder.CreateScenarioAnalyticsInfoBuilder();
        builder.SetVersion("");

        const auto analyticsInfo = builder.Build();
        UNIT_ASSERT_EQUAL("", analyticsInfo.GetVersion());
    }

    Y_UNIT_TEST(SetSemanticFrameInAnalyticsInfoBuilder) {
        TAnalyticsInfoBuilder builder;

        auto frame = CreateSemanticFrame("intent_from_frame");

        builder.SetSemanticFrame(frame);

        const auto json = JsonFromString(ANALYTICS_INFO_SEMANTIC_FRAME_JSON);
        TAnalyticsInfo expected;
        UNIT_ASSERT(JsonToProto(json, expected).ok());

        const auto analyticsInfo = builder.Build();
        UNIT_ASSERT_MESSAGES_EQUAL(expected, analyticsInfo);
    }

    Y_UNIT_TEST(SetSemanticFrameWithoutIntentInAnalyticsInfoBuilder) {
        TAnalyticsInfoBuilder builder;

        auto frame = CreateSemanticFrame();

        builder.SetSemanticFrame(frame);

        const auto json = JsonFromString(ANALYTICS_INFO_SEMANTIC_FRAME_WITHOUT_INTENT_JSON);
        TAnalyticsInfo expected;
        UNIT_ASSERT(JsonToProto(json, expected).ok());

        const auto analyticsInfo = builder.Build();
        UNIT_ASSERT_MESSAGES_EQUAL(expected, analyticsInfo);
    }

    Y_UNIT_TEST(SetSemanticFrameInAnalyticsInfoBuilderWhenScenarioAnalyticsInfoWasCreated) {
        TAnalyticsInfoBuilder builder;

        auto frame = CreateSemanticFrame("intent_from_frame");

        builder.CreateScenarioAnalyticsInfoBuilder()->SetIntentName("intent_scenario_name");
        builder.SetSemanticFrame(frame);

        const auto json = JsonFromString(ANALYTICS_INFO_WITH_SCENARIO_AND_SEMANTIC_FRAME_JSON);
        TAnalyticsInfo expected;
        UNIT_ASSERT(JsonToProto(json, expected).ok());

        const auto analyticsInfo = builder.Build();
        UNIT_ASSERT_MESSAGES_EQUAL(expected, analyticsInfo);
    }

    Y_UNIT_TEST(SetSemanticFrameWithoutIntentInAnalyticsInfoBuilderWhenScenarioAnalyticsInfoWasCreated) {
        TAnalyticsInfoBuilder builder;

        auto frame = CreateSemanticFrame();

        builder.CreateScenarioAnalyticsInfoBuilder()->SetIntentName("intent_scenario_name");
        builder.SetSemanticFrame(frame);

        const auto json = JsonFromString(ANALYTICS_INFO_WITH_SCENARIO_AND_SEMANTIC_FRAME_WITHOUT_INTENT_JSON);
        TAnalyticsInfo expected;
        UNIT_ASSERT(JsonToProto(json, expected).ok());

        const auto analyticsInfo = builder.Build();
        UNIT_ASSERT_MESSAGES_EQUAL(expected, analyticsInfo);
    }

    Y_UNIT_TEST(CheckFillingUserInfoBuilder) {
        TUserInfoBuilder builder;
        builder.CreateScenarioUserInfoBuilder()
            ->AddProfile("id", "name", "description")
            ->AddParams("profile.key.1", "profile.value.1", "profile.description.1")
            .AddParams("profile.key.2", "profile.value.2", "profile.description.2");

        const auto json = JsonFromString(USER_INFO_JSON);
        TUserInfo expected;
        UNIT_ASSERT(JsonToProto(json, expected).ok());

        const auto userInfo = std::move(builder).Build();
        UNIT_ASSERT(userInfo);
        UNIT_ASSERT_MESSAGES_EQUAL(expected, *userInfo);
    }

    Y_UNIT_TEST(CheckFillingUserInfoBuilderOnSecondTime) {
        TUserInfoBuilder builder;
        builder.CreateScenarioUserInfoBuilder()
            ->AddProfile("id", "name", "description")
            ->AddParams("profile.key.1", "profile.value.1", "profile.description.1")
            .AddParams("profile.key.2", "profile.value.2", "profile.description.2");

        UNIT_ASSERT(std::move(builder).Build());

        builder.CreateScenarioUserInfoBuilder()
            ->AddProfile("id.2", "name_2", "description 2")
            ->AddParams("profile.key.3", "profile.value.3", "profile.description.3")
            .AddParams("profile.key.4", "profile.value.4", "profile.description.4");

        const auto json = JsonFromString(USER_INFO_2_JSON);
        TUserInfo expected;
        UNIT_ASSERT(JsonToProto(json, expected).ok());

        const auto userInfo = std::move(builder).Build();
        UNIT_ASSERT(userInfo);
        UNIT_ASSERT_MESSAGES_EQUAL(expected, *userInfo);
    }

    Y_UNIT_TEST(EmptyUserInfoBuilder) {
        TUserInfoBuilder builder;

        UNIT_ASSERT(!std::move(builder).Build());
    }

    Y_UNIT_TEST(NotEmptyUserInfoBuilderWithCreatedScenarioUserInfoBuilder) {
        TUserInfoBuilder builder;
        builder.CreateScenarioUserInfoBuilder();

        UNIT_ASSERT(std::move(builder).Build());
    }

    Y_UNIT_TEST(NotEmptyUserInfoBuilderWithCreatedScenarioUserInfoBuilderAndSettedIntentName) {
        TUserInfoBuilder builder;
        builder.CreateScenarioUserInfoBuilder()->AddProfile("profile_id", "profile_name", "profile_description");

        UNIT_ASSERT(std::move(builder).Build());
    }
}

} // namespace NAlice::NMegamind
