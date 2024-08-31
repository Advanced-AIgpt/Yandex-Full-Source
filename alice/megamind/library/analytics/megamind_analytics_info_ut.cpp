#include "megamind_analytics_info.h"

#include <alice/library/json/json.h>
#include <alice/library/unittest/message_diff.h>

#include <alice/megamind/protos/proactivity/proactivity.pb.h>
#include <alice/megamind/protos/property/property.pb.h>
#include <alice/megamind/protos/scenarios/analytics_info.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>
#include <alice/megamind/protos/scenarios/user_info.pb.h>

#include <alice/protos/data/device/info.pb.h>
#include <alice/protos/data/location/group.pb.h>
#include <alice/protos/data/location/room.pb.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NMegamind {

namespace {

const TString ANALYTICS_SCENARIO_NAME = "cool_scenario";
const TString ANALYTICS_PRODUCT_SCENARIO_NAME = "cool_product_scenario";
const TString USER_SCENARIO_NAME = "vins";

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

constexpr TStringBuf MEGAMIND_ANALYTICS_INFO_JSON = TStringBuf(R"(
{
  "original_utterance": "American rock supergroup",
  "shown_utterance": "American, rock. supergroup!",
  "analytics_info": {
    "cool_scenario": {
      "version": "999",
      "scenario_analytics_info": {
        "objects": [
          {
            "id": "object_1"
          },
          {
            "id": "object_2"
          }
        ],
      }
    }
  },
  "users_info": {
    "vins": {
      "scenario_user_info": {
        "properties": [{
            "id": "id",
            "profile": {}
        }]
      }
    }
  }
}
)");

constexpr TStringBuf MEGAMIND_ANALYTICS_INFO_WITH_SF_JSON = TStringBuf(R"(
{
  "original_utterance": "American rock supergroup",
  "shown_utterance": "American, rock. supergroup!",
  "analytics_info": {
    "cool_scenario": {
      "version": "999",
      "scenario_analytics_info": {
        "objects": [
          {
            "id": "object_1"
          },
          {
            "id": "object_2"
          }
        ],
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
  },
  "users_info": {
    "vins": {
      "scenario_user_info": {
        "properties": [{
            "id": "id",
            "profile": {}
        }]
      }
    }
  }
}
)");

constexpr TStringBuf MEGAMIND_ANALYTICS_INFO_WITH_TUNNELLERS_JSON = TStringBuf(R"(
{
    "tunneller_raw_responses": {
        "scenario_name": {
            "responses": ["tunneller_1", "tunneller_2", "tunneller_3"]
        }
    }
}
)");

constexpr TStringBuf MEGAMIND_ANALYTICS_INFO_WITH_DOUBLE_TUNNELLERS_JSON = TStringBuf(R"(
{
    "tunneller_raw_responses": {
        "scenario_name": {
            "responses": ["tunneller_1", "tunneller_2", "tunneller_3", "tunneller_1", "tunneller_2", "tunneller_3"]
        }
    }
}
)");

constexpr TStringBuf EMPTY_MEGAMIND_ANALYTICS_INFO_JSON = TStringBuf(R"({})");

constexpr TStringBuf MEGAMIND_ANALYTICS_INFO_JSON_REWRITE_USER_INFO = TStringBuf(R"(
{
  "original_utterance": "American rock supergroup",
  "shown_utterance": "American, rock. supergroup!",
  "analytics_info": {
    "cool_scenario": {
      "version": "999",
      "scenario_analytics_info": {
        "objects": [
          {
            "id": "object_1"
          },
          {
            "id": "object_2"
          }
        ],
      }
    }
  },
  "users_info": {
    "vins": {
      "scenario_user_info": {
        "properties": [{
            "id": "id_2",
            "profile": {}
        }]
      }
    }
  }
}
)");

constexpr TStringBuf MEGAMIND_ANALYTICS_INFO_JSON_REWRITE_VERSION_ONLY = TStringBuf(R"(
{
  "original_utterance": "American rock supergroup",
  "shown_utterance": "American, rock. supergroup!",
  "analytics_info": {
    "cool_scenario": {
      "version": "666",
      "scenario_analytics_info": {
        "objects": [
          {
            "id": "object_1"
          },
          {
            "id": "object_2"
          }
        ],
      }
    }
  },
  "users_info": {
    "vins": {
      "scenario_user_info": {
        "properties": [{
            "id": "id",
            "profile": {}
        }]
      }
    }
  }
}
)");

constexpr TStringBuf MEGAMIND_ANALYTICS_INFO_WITH_INTENT_JSON_REWRITE_STATE_ONLY = TStringBuf(R"(
{
  "original_utterance": "American rock supergroup",
  "shown_utterance": "American, rock. supergroup!",
  "analytics_info": {
    "cool_scenario": {
      "version": "999",
      "scenario_analytics_info": {
        "intent": "intent_from_scenario",
        "objects": [
          {
            "id": "object_1"
          },
          {
            "id": "object_2"
          }
        ]
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
  },
  "users_info": {
    "vins": {
      "scenario_user_info": {
        "properties": [{
            "id": "id",
            "profile": {}
        }]
      }
    }
  }
}
)");

constexpr TStringBuf MEGAMIND_ANALYTICS_INFO_JSON_REWRITE_SCENARIO_ANALYTICS_INFO = TStringBuf(R"(
{
  "original_utterance": "American rock supergroup",
  "shown_utterance": "American, rock. supergroup!",
  "analytics_info": {
    "cool_scenario": {
      "version": "666",
      "scenario_analytics_info": {
        "objects": [
          {
            "id": "object_3"
          }
        ],
      }
    }
  },
  "users_info": {
    "vins": {
      "scenario_user_info": {
        "properties": [{
            "id": "id",
            "profile": {}
        }]
      }
    }
  }
}
)");

constexpr TStringBuf MEGAMIND_ANALYTICS_INFO_JSON_FULL_REWRITE_SCENARIO_ANALYTICS_INFO = TStringBuf(R"(
{
  "original_utterance": "American rock supergroup",
  "shown_utterance": "American, rock. supergroup!",
  "analytics_info": {
    "cool_scenario": {
      "version": "666",
      "scenario_analytics_info": {
        "intent" : "intent_from_scenario",
        "objects": [
          {
            "id": "object_3"
          }
        ],
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
  },
  "users_info": {
    "vins": {
      "scenario_user_info": {
        "properties": [{
            "id": "id",
            "profile": {}
        }]
      }
    }
  }
}
)");

constexpr TStringBuf MEGAMIND_ANALYTICS_INFO_JSON_REWRITE_SCENARIO_ANALYTICS_INFO_WITHOUT_VERSION = TStringBuf(R"(
{
  "original_utterance": "American rock supergroup",
  "shown_utterance": "American, rock. supergroup!",
  "analytics_info": {
    "cool_scenario": {
      "version": "999",
      "scenario_analytics_info": {
        "objects": [
          {
            "id": "object_3"
          }
        ],
      }
    }
  },
  "users_info": {
    "vins": {
      "scenario_user_info": {
        "properties": [{
            "id": "id",
            "profile": {}
        }]
      }
    }
  }
}
)");

constexpr TStringBuf MEGAMIND_ANALYTICS_INFO_JSON_VERSION_ONLY = TStringBuf(R"(
{
  "original_utterance": "American rock supergroup",
  "shown_utterance": "American, rock. supergroup!",
  "analytics_info": {
    "cool_scenario": {
      "version": "666"
    }
  },
  "users_info": {
    "vins": {
      "scenario_user_info": {
        "properties": [{
            "id": "id",
            "profile": {}
        }]
      }
    }
  }
}
)");

constexpr TStringBuf MEGAMIND_ANALYTICS_INFO_WITH_MODIFIERS_JSON = TStringBuf(R"(
{
    "modifiers_analytics_info": {
        "proactivity": {
            "appended": true,
            "intent": "personal_assistant.awesome.intent",
            "id": "57"
        }
    },
    "modifiers_info": {
        "proactivity": {
            "appended": true,
            "intent": "personal_assistant.awesome.intent",
            "id": "57"
        }
    }
}
)");

constexpr TStringBuf MEGAMIND_ANALYTICS_INFO_WITH_WINNER_SCENARIO = TStringBuf(R"(
{
    "winner_scenario": {
        "name": "MySuperScenarioName"
    }
}
)");

constexpr TStringBuf MEGAMIND_ANALYTICS_INFO_JSON_EMPTY_SCENARIO = TStringBuf(R"(
{
  "original_utterance": "American rock supergroup",
  "shown_utterance": "American, rock. supergroup!",
  "analytics_info": {
    "cool_scenario": {
    }
  },
  "users_info": {
    "vins": {
      "scenario_user_info": {
        "properties": [{
            "id": "id",
            "profile": {}
        }]
      }
    }
  }
}
)");

constexpr TStringBuf MEGAMIND_ANALYTICS_INFO_JSON_PRODUCT_SCENARIO_NAME_ONLY = TStringBuf(R"(
{
  "original_utterance": "American rock supergroup",
  "shown_utterance": "American, rock. supergroup!",
  "analytics_info": {
    "cool_scenario": {
      "scenario_analytics_info": {
        "product_scenario_name": "cool_product_scenario"
      }
    }
  },
  "users_info": {
    "vins": {
      "scenario_user_info": {
        "properties": [{
            "id": "id",
            "profile": {}
        }]
      }
    }
  }
}
)");

constexpr TStringBuf MEGAMIND_ANALYTICS_INFO_WITH_SCENARIO_TIMINGS = TStringBuf(R"(
{
  "original_utterance": "American rock supergroup",
  "shown_utterance": "American, rock. supergroup!",
  "users_info": {
    "vins": {
      "scenario_user_info": {
        "properties": [
          {
            "profile": {},
            "id": "id"
          }
        ]
      }
    }
  },
  "analytics_info": {
    "cool_scenario": {
      "version": "999",
      "scenario_analytics_info": {
        "objects": [
          {
            "id": "object_1"
          },
          {
            "id": "object_2"
          }
        ]
     }
    }
  },
  "scenario_timings": {
    "cool_scenario": {
      "timings": {
        "run": {
          "start_timestamp": "1594062000000000",
          "source_response_durations": {
            "run-source": "30000"
          }
        }
      }
    }
  }
})");

constexpr TStringBuf MEGAMIND_ANALYTICS_INFO_WITH_IOT_USER_INFO = TStringBuf(R"(
{
  "iot_user_info":{
    "colors":[
      {
        "name":"Малиновый",
        "id":"raspberry"
      }
    ],
    "rooms":[
      {
        "name":"Комната грязи",
        "id":"1337"
      }
    ]
  }
})");

constexpr TStringBuf MEGAMIND_ANALYTICS_INFO_WITH_USER_PROFILE = TStringBuf(R"(
{
  "user_profile":{
    "subscriptions": [
      "kinopoisk"
    ],
    "has_yandex_plus": true
  }
})");

constexpr TStringBuf MEGAMIND_ANALYTICS_INFO_WITH_PARENT_RPODUCT_SCENARIO_NAME_JSON = TStringBuf(R"(
{
  "analytics_info": {
    "cool_scenario": {
      "parent_product_scenario_name": "parent_name",
    }
  },
  "parent_product_scenario_name":"parent_name"
}
)");

constexpr TStringBuf MEGAMIND_ANALYTICS_INFO_WITH_LOCATION_JSON = TStringBuf(R"(
{
  "location": {
    "speed": 0,
    "lat": 1,
    "lon": 2,
    "recency": 0,
    "accuracy": 3
  },
  "analytics_info": {
    "cool_scenario": {}
  }
}
)");

constexpr TStringBuf MEGAMIND_ANALYTICS_INFO_WITH_RECOGNIZED_ACTION = TStringBuf(R"(
{
  "recognized_action": {
    "action_id":"actionID",
    "parent_product_scenario_name":"parentProductScenarioName",
    "parent_request_id":"parentReqID"
  }
}
)");

constexpr auto MEGAMIND_ANALYTICS_INFO_WITH_POSTROLL_FRAME_ACTIONS = TStringBuf(R"(
{
    "modifiers_analytics_info": {
        "postroll": {
            "frame_actions": [
                {
                    "name": "POSTROLL_ACTION"
                }
            ]
        }
    }
}
)");

TAnalyticsInfo CreateDefaultAnalyticsInfo() {
    TAnalyticsInfo analyticsInfo;
    analyticsInfo.SetVersion("999");
    analyticsInfo.MutableScenarioAnalyticsInfo()->AddTunnellerRawResponses("tunneller_1");
    analyticsInfo.MutableScenarioAnalyticsInfo()->AddTunnellerRawResponses("tunneller_2");
    analyticsInfo.MutableScenarioAnalyticsInfo()->AddTunnellerRawResponses("tunneller_3");
    analyticsInfo.MutableScenarioAnalyticsInfo()->AddObjects()->SetId("object_1");
    analyticsInfo.MutableScenarioAnalyticsInfo()->AddObjects()->SetId("object_2");
    return analyticsInfo;
}

TUserInfo CreateDefaultUserInfo() {
    TUserInfo userInfo;
    auto* properties = userInfo.MutableScenarioUserInfo()->AddProperties();
    properties->SetId("id");
    properties->MutableProfile();
    return userInfo;
}

TProactivityInfo CreateDefaultProactivityInfo() {
    TProactivityInfo proactivityInfo;
    proactivityInfo.SetId(57);
    proactivityInfo.SetIntent("personal_assistant.awesome.intent");
    proactivityInfo.SetAppended(true);
    return proactivityInfo;
}

TModifiersInfo CreateDefaultModifiersInfo() {
    TModifiersInfo modifiersInfo;
    *modifiersInfo.MutableProactivity() = CreateDefaultProactivityInfo();
    return modifiersInfo;
}

TMegamindAnalyticsInfoBuilder CreateMegamindAnalyticsInfo(const TAnalyticsInfo& analyticsInfo,
                                                          const TUserInfo& userInfo) {
    TMegamindAnalyticsInfoBuilder megamindAnalyticsInfoBuilder;
    megamindAnalyticsInfoBuilder.SetOriginalUtterance("American rock supergroup")
        .SetShownUtterance("American, rock. supergroup!")
        .AddAnalyticsInfo(ANALYTICS_SCENARIO_NAME, analyticsInfo)
        .AddUserInfo(USER_SCENARIO_NAME, userInfo);
    return megamindAnalyticsInfoBuilder;
}

const TAnalyticsInfo& GetAnalyticsInfo(const TMegamindAnalyticsInfoBuilder& builder, const TString& scenarioName) {
    return builder.BuildProto().GetAnalyticsInfo().at(scenarioName);
}

THashSet<TStringBuf> GetMatchedSemanticFrameNames(const TMegamindAnalyticsInfoBuilder& builder,
                                                  const TString& scenarioName) {
    THashSet<TStringBuf> result;
    for (const auto& frame : GetAnalyticsInfo(builder, scenarioName).GetMatchedSemanticFrames()) {
        result.insert(frame.GetName());
    }
    return result;
}

#define CHECK_MEGAMIND_ANALYTICS_INFO(expected, actual)                                                               \
    do {                                                                                                              \
        const auto expectedJson = JsonFromString(expected);                                                           \
        TMegamindAnalyticsInfo expectedProto;                                                                         \
        UNIT_ASSERT(JsonToProto(expectedJson, expectedProto).ok());                                                   \
        UNIT_ASSERT_VALUES_EQUAL(expectedJson, (actual).BuildJson());                                                 \
        UNIT_ASSERT_MESSAGES_EQUAL(expectedProto, (actual).BuildProto());                                             \
    } while (false)

} // namespace

Y_UNIT_TEST_SUITE(MegamindAnalyticsInfoBuilder) {
    Y_UNIT_TEST(FillingMegamindAnalyticsInfoBuilder) {
        auto analyticsInfo = CreateDefaultAnalyticsInfo();
        auto userInfo = CreateDefaultUserInfo();

        auto megamindAnalyticsInfoBuilder = CreateMegamindAnalyticsInfo(analyticsInfo, userInfo);

        CHECK_MEGAMIND_ANALYTICS_INFO(MEGAMIND_ANALYTICS_INFO_JSON, megamindAnalyticsInfoBuilder);
    }

    Y_UNIT_TEST(AddProductScenarioNameInMegamindAnalyticsInfoBuilder) {
        auto userInfo = CreateDefaultUserInfo();
        TAnalyticsInfo analyticsInfo;
        analyticsInfo.MutableScenarioAnalyticsInfo()->SetProductScenarioName(ANALYTICS_PRODUCT_SCENARIO_NAME);

        auto megamindAnalyticsInfoBuilder = CreateMegamindAnalyticsInfo(analyticsInfo, userInfo);

        CHECK_MEGAMIND_ANALYTICS_INFO(MEGAMIND_ANALYTICS_INFO_JSON_PRODUCT_SCENARIO_NAME_ONLY,
                                      megamindAnalyticsInfoBuilder);
    }

    Y_UNIT_TEST(AddEmptyScenarioAnalyticsInfoInMegamindAnalyticsInfoBuilderFirstTime) {
        auto userInfo = CreateDefaultUserInfo();
        TAnalyticsInfo emptyAnalyticsInfo;
        emptyAnalyticsInfo.MutableScenarioAnalyticsInfo();

        auto megamindAnalyticsInfoBuilder = CreateMegamindAnalyticsInfo(emptyAnalyticsInfo, userInfo);

        CHECK_MEGAMIND_ANALYTICS_INFO(MEGAMIND_ANALYTICS_INFO_JSON_EMPTY_SCENARIO, megamindAnalyticsInfoBuilder);
    }

    Y_UNIT_TEST(AddEmptyScenarioAnalyticsInfoInMegamindAnalyticsInfoBuilderSecondTime) {
        auto analyticsInfo = CreateDefaultAnalyticsInfo();
        *analyticsInfo.MutableSemanticFrame() = CreateSemanticFrame("intent_from_frame");
        auto userInfo = CreateDefaultUserInfo();
        TAnalyticsInfo emptyAnalyticsInfo;
        emptyAnalyticsInfo.MutableScenarioAnalyticsInfo();

        auto megamindAnalyticsInfoBuilder = CreateMegamindAnalyticsInfo(analyticsInfo, userInfo)
                .AddAnalyticsInfo(ANALYTICS_SCENARIO_NAME, emptyAnalyticsInfo);

        CHECK_MEGAMIND_ANALYTICS_INFO(MEGAMIND_ANALYTICS_INFO_WITH_SF_JSON, megamindAnalyticsInfoBuilder);
    }

    Y_UNIT_TEST(AddEmptyAnalyticsInfoInMegamindAnalyticsInfoBuilderSecondTime) {
        auto analyticsInfo = CreateDefaultAnalyticsInfo();
        *analyticsInfo.MutableSemanticFrame() = CreateSemanticFrame("intent_from_frame");
        auto userInfo = CreateDefaultUserInfo();
        TAnalyticsInfo emptyAnalyticsInfo;

        auto megamindAnalyticsInfoBuilder = CreateMegamindAnalyticsInfo(analyticsInfo, userInfo)
            .AddAnalyticsInfo(ANALYTICS_SCENARIO_NAME, emptyAnalyticsInfo);

        CHECK_MEGAMIND_ANALYTICS_INFO(MEGAMIND_ANALYTICS_INFO_WITH_SF_JSON, megamindAnalyticsInfoBuilder);
    }

    Y_UNIT_TEST(AddNotEmptyVersionInMegamindAnalyticsInfoBuilder) {
        auto analyticsInfo = CreateDefaultAnalyticsInfo();
        auto userInfo = CreateDefaultUserInfo();
        TAnalyticsInfo secondAnalyticsInfo;
        secondAnalyticsInfo.SetVersion("666");

        auto megamindAnalyticsInfoBuilder = CreateMegamindAnalyticsInfo(analyticsInfo, userInfo)
                .AddAnalyticsInfo(ANALYTICS_SCENARIO_NAME, secondAnalyticsInfo);

        CHECK_MEGAMIND_ANALYTICS_INFO(MEGAMIND_ANALYTICS_INFO_JSON_REWRITE_VERSION_ONLY, megamindAnalyticsInfoBuilder);
    }

    Y_UNIT_TEST(AddNotEmptyStateInMegamindAnalyticsInfoBuilderWithIntent) {
        auto analyticsInfo = CreateDefaultAnalyticsInfo();
        analyticsInfo.MutableScenarioAnalyticsInfo()->SetIntent("intent_from_scenario");
        auto userInfo = CreateDefaultUserInfo();
        TAnalyticsInfo secondAnalyticsInfo;
        *secondAnalyticsInfo.MutableSemanticFrame() = CreateSemanticFrame("intent_from_frame");

        auto megamindAnalyticsInfoBuilder = CreateMegamindAnalyticsInfo(analyticsInfo, userInfo)
                .AddAnalyticsInfo(ANALYTICS_SCENARIO_NAME, secondAnalyticsInfo);

        CHECK_MEGAMIND_ANALYTICS_INFO(MEGAMIND_ANALYTICS_INFO_WITH_INTENT_JSON_REWRITE_STATE_ONLY,
                                      megamindAnalyticsInfoBuilder);
    }

    Y_UNIT_TEST(AddEmptyScenarioAnalyticsInfoAndNotEmptyVersionInMegamindAnalyticsInfoBuilder) {
        auto analyticsInfo = CreateDefaultAnalyticsInfo();
        auto userInfo = CreateDefaultUserInfo();
        TAnalyticsInfo secondAnalyticsInfo;
        secondAnalyticsInfo.MutableScenarioAnalyticsInfo();
        secondAnalyticsInfo.SetVersion("666");

        auto megamindAnalyticsInfoBuilder = CreateMegamindAnalyticsInfo(analyticsInfo, userInfo)
                .AddAnalyticsInfo(ANALYTICS_SCENARIO_NAME, secondAnalyticsInfo);

        CHECK_MEGAMIND_ANALYTICS_INFO(MEGAMIND_ANALYTICS_INFO_JSON_REWRITE_VERSION_ONLY, megamindAnalyticsInfoBuilder);
    }

    Y_UNIT_TEST(AddNotEmptyScenarioAnalyticsInfoAndNotEmptyVersionInMegamindAnalyticsInfoBuilder) {
        auto analyticsInfo = CreateDefaultAnalyticsInfo();
        auto userInfo = CreateDefaultUserInfo();
        TAnalyticsInfo secondAnalyticsInfo;
        secondAnalyticsInfo.MutableScenarioAnalyticsInfo()->AddObjects()->SetId("object_3");
        secondAnalyticsInfo.SetVersion("666");

        auto megamindAnalyticsInfoBuilder = CreateMegamindAnalyticsInfo(analyticsInfo, userInfo)
                .AddAnalyticsInfo(ANALYTICS_SCENARIO_NAME, secondAnalyticsInfo);

        CHECK_MEGAMIND_ANALYTICS_INFO(MEGAMIND_ANALYTICS_INFO_JSON_REWRITE_SCENARIO_ANALYTICS_INFO,
                                      megamindAnalyticsInfoBuilder);
    }

    Y_UNIT_TEST(AddNotEmptyScenarioAnalyticsInfoInMegamindAnalyticsInfoBuilder) {
        auto analyticsInfo = CreateDefaultAnalyticsInfo();
        auto userInfo = CreateDefaultUserInfo();
        TAnalyticsInfo secondAnalyticsInfo;
        secondAnalyticsInfo.MutableScenarioAnalyticsInfo()->SetIntent("intent_from_scenario");
        secondAnalyticsInfo.MutableScenarioAnalyticsInfo()->AddObjects()->SetId("object_3");
        secondAnalyticsInfo.SetVersion("666");
        *secondAnalyticsInfo.MutableSemanticFrame() = CreateSemanticFrame("intent_from_frame");

        auto megamindAnalyticsInfoBuilder = CreateMegamindAnalyticsInfo(analyticsInfo, userInfo)
                .AddAnalyticsInfo(ANALYTICS_SCENARIO_NAME, secondAnalyticsInfo);

        CHECK_MEGAMIND_ANALYTICS_INFO(MEGAMIND_ANALYTICS_INFO_JSON_FULL_REWRITE_SCENARIO_ANALYTICS_INFO,
                                      megamindAnalyticsInfoBuilder);
    }

    Y_UNIT_TEST(AddNotEmptyScenarioAnalyticsInfoWithoutVersionInMegamindAnalyticsInfoBuilder) {
        auto analyticsInfo = CreateDefaultAnalyticsInfo();
        auto userInfo = CreateDefaultUserInfo();
        TAnalyticsInfo secondAnalyticsInfo;
        secondAnalyticsInfo.MutableScenarioAnalyticsInfo()->AddObjects()->SetId("object_3");

        auto megamindAnalyticsInfoBuilder = CreateMegamindAnalyticsInfo(analyticsInfo, userInfo)
                .AddAnalyticsInfo(ANALYTICS_SCENARIO_NAME, secondAnalyticsInfo);

        CHECK_MEGAMIND_ANALYTICS_INFO(MEGAMIND_ANALYTICS_INFO_JSON_REWRITE_SCENARIO_ANALYTICS_INFO_WITHOUT_VERSION,
                                      megamindAnalyticsInfoBuilder);
    }

    Y_UNIT_TEST(AddNotEmptyScenarioAnalyticsInfoInMegamindAnalyticsInfoBuilder2) {
        auto userInfo = CreateDefaultUserInfo();
        TAnalyticsInfo emptyAnalyticsInfo;
        emptyAnalyticsInfo.SetVersion("666");

        auto megamindAnalyticsInfoBuilder = CreateMegamindAnalyticsInfo(emptyAnalyticsInfo, userInfo);

        CHECK_MEGAMIND_ANALYTICS_INFO(MEGAMIND_ANALYTICS_INFO_JSON_VERSION_ONLY, megamindAnalyticsInfoBuilder);
    }

    Y_UNIT_TEST(AddEmptyUserInfoInMegamindAnalyticsInfoBuilder) {
        auto analyticsInfo = CreateDefaultAnalyticsInfo();
        auto userInfo = CreateDefaultUserInfo();
        TUserInfo emptyUserInfo;

        auto megamindAnalyticsInfoBuilder = CreateMegamindAnalyticsInfo(analyticsInfo, userInfo)
                .AddUserInfo(USER_SCENARIO_NAME, emptyUserInfo);

        CHECK_MEGAMIND_ANALYTICS_INFO(MEGAMIND_ANALYTICS_INFO_JSON, megamindAnalyticsInfoBuilder);
    }

    Y_UNIT_TEST(AddEmptyScenarioUserInfoInMegamindAnalyticsInfoBuilder) {
        auto analyticsInfo = CreateDefaultAnalyticsInfo();
        auto userInfo = CreateDefaultUserInfo();
        TUserInfo emptyUserInfo;
        emptyUserInfo.MutableScenarioUserInfo();

        auto megamindAnalyticsInfoBuilder = CreateMegamindAnalyticsInfo(analyticsInfo, userInfo)
                .AddUserInfo(USER_SCENARIO_NAME, emptyUserInfo);

        CHECK_MEGAMIND_ANALYTICS_INFO(MEGAMIND_ANALYTICS_INFO_JSON, megamindAnalyticsInfoBuilder);
    }

    Y_UNIT_TEST(AddNotEmptyScenarioUserInfoInMegamindAnalyticsInfoBuilder) {
        auto analyticsInfo = CreateDefaultAnalyticsInfo();
        auto userInfo = CreateDefaultUserInfo();
        TUserInfo secondUserInfo;
        auto* properties = secondUserInfo.MutableScenarioUserInfo()->AddProperties();
        properties->SetId("id_2");
        properties->MutableProfile();

        auto megamindAnalyticsInfoBuilder = CreateMegamindAnalyticsInfo(analyticsInfo, userInfo)
                .AddUserInfo(USER_SCENARIO_NAME, secondUserInfo);

        CHECK_MEGAMIND_ANALYTICS_INFO(MEGAMIND_ANALYTICS_INFO_JSON_REWRITE_USER_INFO, megamindAnalyticsInfoBuilder);
    }

    Y_UNIT_TEST(CopyTunnellerRawResponses) {
        auto analyticsInfo = CreateDefaultAnalyticsInfo();

        NMegamind::TMegamindAnalyticsInfoBuilder megamindAnalyticsInfoBuilder;
        megamindAnalyticsInfoBuilder.CopyTunnellerRawResponses("scenario_name", analyticsInfo);

        CHECK_MEGAMIND_ANALYTICS_INFO(MEGAMIND_ANALYTICS_INFO_WITH_TUNNELLERS_JSON, megamindAnalyticsInfoBuilder);
    }

    Y_UNIT_TEST(DoubleCopyTunnellerRawResponses) {
        auto analyticsInfo = CreateDefaultAnalyticsInfo();

        NMegamind::TMegamindAnalyticsInfoBuilder megamindAnalyticsInfoBuilder;
        megamindAnalyticsInfoBuilder.CopyTunnellerRawResponses("scenario_name", analyticsInfo);
        megamindAnalyticsInfoBuilder.CopyTunnellerRawResponses("scenario_name", analyticsInfo);

        CHECK_MEGAMIND_ANALYTICS_INFO(MEGAMIND_ANALYTICS_INFO_WITH_DOUBLE_TUNNELLERS_JSON,
                                      megamindAnalyticsInfoBuilder);
    }

    Y_UNIT_TEST(DoubleCopyTunnellerRawResponses2) {
        NMegamind::TMegamindAnalyticsInfoBuilder megamindAnalyticsInfoBuilder;

        {
            auto analyticsInfo = CreateDefaultAnalyticsInfo();
            megamindAnalyticsInfoBuilder.CopyTunnellerRawResponses("scenario_name", analyticsInfo);
        }

        {
            auto analyticsInfo = CreateDefaultAnalyticsInfo();
            megamindAnalyticsInfoBuilder.CopyTunnellerRawResponses("scenario_name", analyticsInfo);
        }

        CHECK_MEGAMIND_ANALYTICS_INFO(MEGAMIND_ANALYTICS_INFO_WITH_DOUBLE_TUNNELLERS_JSON,
                                      megamindAnalyticsInfoBuilder);
    }

    Y_UNIT_TEST(CopyTunnellerRawResponsesWithEmptyAnalyticsInfo) {
        TAnalyticsInfo analyticsInfo;

        NMegamind::TMegamindAnalyticsInfoBuilder megamindAnalyticsInfoBuilder;
        megamindAnalyticsInfoBuilder.CopyTunnellerRawResponses("scenario_name", analyticsInfo);

        CHECK_MEGAMIND_ANALYTICS_INFO(EMPTY_MEGAMIND_ANALYTICS_INFO_JSON, megamindAnalyticsInfoBuilder);
    }

    Y_UNIT_TEST(CopyTunnellerRawResponsesWithoutScenarioAnalyticsInfo) {
        TAnalyticsInfo analyticsInfo;
        analyticsInfo.SetVersion("2020");

        NMegamind::TMegamindAnalyticsInfoBuilder megamindAnalyticsInfoBuilder;
        megamindAnalyticsInfoBuilder.CopyTunnellerRawResponses("scenario_name", analyticsInfo);

        CHECK_MEGAMIND_ANALYTICS_INFO(EMPTY_MEGAMIND_ANALYTICS_INFO_JSON, megamindAnalyticsInfoBuilder);
    }

    Y_UNIT_TEST(CopyTunnellerRawResponsesWithoutTunnellerResponses) {
        auto analyticsInfo = CreateDefaultAnalyticsInfo();
        analyticsInfo.MutableScenarioAnalyticsInfo()->ClearTunnellerRawResponses();

        NMegamind::TMegamindAnalyticsInfoBuilder megamindAnalyticsInfoBuilder;
        megamindAnalyticsInfoBuilder.CopyTunnellerRawResponses("scenario_name", analyticsInfo);

        CHECK_MEGAMIND_ANALYTICS_INFO(EMPTY_MEGAMIND_ANALYTICS_INFO_JSON, megamindAnalyticsInfoBuilder);
    }

    Y_UNIT_TEST(SetModifiersInfo) {
        auto modifiersInfo = CreateDefaultModifiersInfo();

        NMegamind::TMegamindAnalyticsInfoBuilder megamindAnalyticsInfoBuilder;
        megamindAnalyticsInfoBuilder.SetModifiersInfo(modifiersInfo);

        CHECK_MEGAMIND_ANALYTICS_INFO(MEGAMIND_ANALYTICS_INFO_WITH_MODIFIERS_JSON, megamindAnalyticsInfoBuilder);
    }

    Y_UNIT_TEST(AddScenarioTimings) {
        auto analyticsInfo = CreateDefaultAnalyticsInfo();
        auto userInfo = CreateDefaultUserInfo();
        TAnalyticsInfo secondAnalyticsInfo;
        auto& timings =
            (*secondAnalyticsInfo.MutableScenarioAnalyticsInfo()->MutableScenarioTimings()->MutableTimings())["run"];
        timings.SetStartTimestamp(TInstant::ParseIso8601("2020-07-06T19:00:00Z").MicroSeconds());
        (*timings.MutableSourceResponseDurations())["run-source"] = 30000;

        auto megamindAnalyticsInfoBuilder = CreateMegamindAnalyticsInfo(analyticsInfo, userInfo)
                                                .AddScenarioTimings(ANALYTICS_SCENARIO_NAME, secondAnalyticsInfo);

        CHECK_MEGAMIND_ANALYTICS_INFO(MEGAMIND_ANALYTICS_INFO_WITH_SCENARIO_TIMINGS, megamindAnalyticsInfoBuilder);
    }

    Y_UNIT_TEST(SetWinnerScenarioName) {
        auto modifiersInfo = CreateDefaultModifiersInfo();

        NMegamind::TMegamindAnalyticsInfoBuilder megamindAnalyticsInfoBuilder;
        megamindAnalyticsInfoBuilder.SetWinnerScenarioName("MySuperScenarioName");

        CHECK_MEGAMIND_ANALYTICS_INFO(MEGAMIND_ANALYTICS_INFO_WITH_WINNER_SCENARIO, megamindAnalyticsInfoBuilder);
    }

    Y_UNIT_TEST(MatchedSemanticFramesAfterApplyWithScenarioAnalyticsInfoInRun) {
        TAnalyticsInfo analyticsInfoRun = CreateDefaultAnalyticsInfo();
        {
            TSemanticFrame semanticFrame;
            semanticFrame.SetName("lol");
            *analyticsInfoRun.AddMatchedSemanticFrames() = semanticFrame;
        }
        {
            TSemanticFrame semanticFrame;
            semanticFrame.SetName("kek");
            *analyticsInfoRun.AddMatchedSemanticFrames() = semanticFrame;
        }
        auto userInfo = CreateDefaultUserInfo();

        auto megamindAnalyticsInfoBuilder = CreateMegamindAnalyticsInfo(analyticsInfoRun, userInfo);

        megamindAnalyticsInfoBuilder.AddAnalyticsInfo(ANALYTICS_SCENARIO_NAME, CreateDefaultAnalyticsInfo());

        auto actual = GetMatchedSemanticFrameNames(megamindAnalyticsInfoBuilder, ANALYTICS_SCENARIO_NAME);
        THashSet<TStringBuf> expected = {"lol", "kek"};

        UNIT_ASSERT_VALUES_EQUAL(expected, actual);
    }

    Y_UNIT_TEST(MatchedSemanticFramesAfterApplyWithoutScenarioAnalyticsInfoInRun) {
        TAnalyticsInfo analyticsInfoRun;
        {
            TSemanticFrame semanticFrame;
            semanticFrame.SetName("lol");
            *analyticsInfoRun.AddMatchedSemanticFrames() = semanticFrame;
        }
        {
            TSemanticFrame semanticFrame;
            semanticFrame.SetName("kek");
            *analyticsInfoRun.AddMatchedSemanticFrames() = semanticFrame;
        }
        auto userInfo = CreateDefaultUserInfo();

        auto megamindAnalyticsInfoBuilder = CreateMegamindAnalyticsInfo(analyticsInfoRun, userInfo);

        megamindAnalyticsInfoBuilder.AddAnalyticsInfo(ANALYTICS_SCENARIO_NAME, CreateDefaultAnalyticsInfo());

        auto actual = GetMatchedSemanticFrameNames(megamindAnalyticsInfoBuilder, ANALYTICS_SCENARIO_NAME);
        THashSet<TStringBuf> expected = {"lol", "kek"};

        UNIT_ASSERT_VALUES_EQUAL(expected, actual);
    }

    Y_UNIT_TEST(AddFrameActionsInMegamindAnalyticsInfo) {
        TAnalyticsInfo analyticsInfoRun;
        {
            NScenarios::TFrameAction frameAction;
            frameAction.MutableNluHint()->SetFrameName("lol");
            (*analyticsInfoRun.MutableFrameActions())["lol_key"] = frameAction;
        }
        {
            NScenarios::TFrameAction frameAction;
            frameAction.MutableNluHint()->SetFrameName("kek");
            (*analyticsInfoRun.MutableFrameActions())["kek_key"] = frameAction;
        }

        TAnalyticsInfo analyticsInfoApply;
        {
            NScenarios::TFrameAction frameAction;
            frameAction.MutableNluHint()->SetFrameName("foo");
            (*analyticsInfoApply.MutableFrameActions())["foo_key"] = frameAction;
        }
        {
            NScenarios::TFrameAction frameAction;
            frameAction.MutableNluHint()->SetFrameName("bar");
            (*analyticsInfoApply.MutableFrameActions())["bar_key"] = frameAction;
        }

        auto userInfo = CreateDefaultUserInfo();

        THashMap<TStringBuf, TStringBuf> expectedRun = {{"lol_key", "lol"}, {"kek_key", "kek"}};
        THashMap<TStringBuf, TStringBuf> expectedApply = {{"foo_key", "foo"}, {"bar_key", "bar"}};

        auto getFrames = [](const TMegamindAnalyticsInfoBuilder& builder, const TString& scenarioName) {
            THashMap<TStringBuf, TStringBuf> actual;
            for (const auto& frame : GetAnalyticsInfo(builder, scenarioName).GetFrameActions()) {
                actual[frame.first] = frame.second.GetNluHint().GetFrameName();
            }
            return actual;
        };

        auto megamindAnalyticsInfoBuilder = CreateMegamindAnalyticsInfo(analyticsInfoRun, userInfo);

        {
            auto actual = getFrames(megamindAnalyticsInfoBuilder, ANALYTICS_SCENARIO_NAME);
            UNIT_ASSERT_VALUES_EQUAL(expectedRun, actual);
        }

        {
            auto defaultAnalyticsInfo = CreateDefaultAnalyticsInfo();
            UNIT_ASSERT_C(defaultAnalyticsInfo.GetFrameActions().empty(), "FrameActions must be empty");
            megamindAnalyticsInfoBuilder.AddAnalyticsInfo(ANALYTICS_SCENARIO_NAME, defaultAnalyticsInfo);

            auto actual = getFrames(megamindAnalyticsInfoBuilder, ANALYTICS_SCENARIO_NAME);
            UNIT_ASSERT_VALUES_EQUAL(expectedRun, actual);
        }

        {
            megamindAnalyticsInfoBuilder.AddAnalyticsInfo(ANALYTICS_SCENARIO_NAME, analyticsInfoApply);

            auto actual = getFrames(megamindAnalyticsInfoBuilder, ANALYTICS_SCENARIO_NAME);
            UNIT_ASSERT_VALUES_EQUAL(expectedApply, actual);
        }
    }

    Y_UNIT_TEST(SetIoTUserInfo) {
        NMegamind::TMegamindAnalyticsInfoBuilder megamindAnalyticsInfoBuilder;
        TIoTUserInfo ioTUserInfo;
        {
            auto* color = ioTUserInfo.AddColors();
            color->SetId("raspberry");
            color->SetName("Малиновый");
        }
        {
            auto* room = ioTUserInfo.AddRooms();
            room->SetId("1337");
            room->SetName("Комната грязи");
        }
        megamindAnalyticsInfoBuilder.SetIoTUserInfo(ioTUserInfo);

        CHECK_MEGAMIND_ANALYTICS_INFO(MEGAMIND_ANALYTICS_INFO_WITH_IOT_USER_INFO, megamindAnalyticsInfoBuilder);
    }

    Y_UNIT_TEST(SetUserProfile) {
        NMegamind::TMegamindAnalyticsInfoBuilder megamindAnalyticsInfoBuilder;
        TUserProfile userProfile;
        userProfile.AddSubscriptions("kinopoisk");
        userProfile.SetHasYandexPlus(true);
        megamindAnalyticsInfoBuilder.SetUserProfile(userProfile);

        CHECK_MEGAMIND_ANALYTICS_INFO(MEGAMIND_ANALYTICS_INFO_WITH_USER_PROFILE, megamindAnalyticsInfoBuilder);
    }

    Y_UNIT_TEST(SetParentProductScenarioName) {
        TAnalyticsInfo analyticsInfo;

        TMegamindAnalyticsInfoBuilder megamindAnalyticsInfoBuilder;
        megamindAnalyticsInfoBuilder.SetParentProductScenarioName("parent_name");
        megamindAnalyticsInfoBuilder.AddAnalyticsInfo(ANALYTICS_SCENARIO_NAME, analyticsInfo);

        CHECK_MEGAMIND_ANALYTICS_INFO(MEGAMIND_ANALYTICS_INFO_WITH_PARENT_RPODUCT_SCENARIO_NAME_JSON,
                                      megamindAnalyticsInfoBuilder);
    }

    Y_UNIT_TEST(SetLocation) {
        TAnalyticsInfo analyticsInfo;

        TMegamindAnalyticsInfoBuilder megamindAnalyticsInfoBuilder;
        const TRequest::TLocation location{/* latitude= */ 1, /* longitude= */ 2, /* accuracy= */ 3};
        megamindAnalyticsInfoBuilder.SetLocation(location);
        megamindAnalyticsInfoBuilder.AddAnalyticsInfo(ANALYTICS_SCENARIO_NAME, analyticsInfo);

        CHECK_MEGAMIND_ANALYTICS_INFO(MEGAMIND_ANALYTICS_INFO_WITH_LOCATION_JSON,
                                      megamindAnalyticsInfoBuilder);
    }

    Y_UNIT_TEST(SetRecognizedScenarioAction) {
        TMegamindAnalyticsInfoBuilder megamindAnalyticsInfoBuilder;
        megamindAnalyticsInfoBuilder.SetRecognizedScenarioAction("parentReqID", "actionID", "parentProductScenarioName");

        CHECK_MEGAMIND_ANALYTICS_INFO(MEGAMIND_ANALYTICS_INFO_WITH_RECOGNIZED_ACTION,
                                      megamindAnalyticsInfoBuilder);
    }

    Y_UNIT_TEST(SetPostroll) {
        TMegamindAnalyticsInfoBuilder megamindAnalyticsInfoBuilder;
        NModifiers::NProactivity::TPostroll postroll{};
        postroll.AddFrameActions()->SetName("POSTROLL_ACTION");
        megamindAnalyticsInfoBuilder.SetPostroll(postroll);

        CHECK_MEGAMIND_ANALYTICS_INFO(MEGAMIND_ANALYTICS_INFO_WITH_POSTROLL_FRAME_ACTIONS,
                                      megamindAnalyticsInfoBuilder);
    }
}

} // namespace NAlice::NMegamind
