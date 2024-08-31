#include "walker.h"

#include <alice/megamind/library/apphost_request/protos/scenario_errors.pb.h>
#include <alice/megamind/library/apphost_request/item_names.h>
#include <alice/megamind/library/factor_storage/factor_storage.h>
#include <alice/megamind/library/handlers/apphost_megamind/combinators.h>
#include <alice/megamind/library/memento/memento.h>
#include <alice/megamind/library/request/event/server_action_event.h>
#include <alice/megamind/library/request/event/text_input_event.h>
#include <alice/megamind/library/response/utils.h>
#include <alice/megamind/library/scenarios/helpers/scenario_wrapper.h>
#include <alice/megamind/library/scenarios/protocol/protocol_scenario.h>
#include <alice/megamind/library/testing/apphost_helpers.h>
#include <alice/megamind/library/testing/fake_modifier.h>
#include <alice/megamind/library/testing/mock_context.h>
#include <alice/megamind/library/testing/mock_data_sources.h>
#include <alice/megamind/library/testing/mock_global_context.h>
#include <alice/megamind/library/testing/mock_modifier_request_factory.h>
#include <alice/megamind/library/testing/mock_postclassify_state.h>
#include <alice/megamind/library/testing/mock_request_context.h>
#include <alice/megamind/library/testing/mock_responses.h>
#include <alice/megamind/library/testing/mock_scenario_wrapper.h>
#include <alice/megamind/library/testing/mock_session.h>
#include <alice/megamind/library/testing/mock_walker_request.h>
#include <alice/megamind/library/testing/speechkit.h>
#include <alice/megamind/library/testing/utils.h>
#include <alice/megamind/nlg/register.h>

#include <alice/megamind/protos/common/effect_options.pb.h>
#include <alice/megamind/protos/modifiers/modifier_body.pb.h>
#include <alice/megamind/protos/modifiers/modifier_response.pb.h>
#include <alice/megamind/protos/scenarios/combinator_response.pb.h>
#include <alice/megamind/protos/scenarios/request.pb.h>
#include <alice/megamind/protos/scenarios/stack_engine.pb.h>

#include <alice/protos/data/language/language.pb.h>
#include <alice/protos/data/scenario/music/content_id.pb.h>

#include <alice/bass/libs/fetcher/util.h>
#include <alice/library/experiments/flags.h>
#include <alice/library/frame/builder.h>
#include <alice/library/geo/user_location.h>
#include <alice/library/iot/iot.h>
#include <alice/library/metrics/util.h>
#include <alice/library/typed_frame/typed_semantic_frame_request.h>
#include <alice/library/unittest/fake_fetcher.h>
#include <alice/library/unittest/message_diff.h>
#include <alice/library/unittest/mock_request_eventlistener.h>
#include <alice/library/unittest/mock_sensors.h>
#include <alice/library/version/version.h>

#include <alice/nlg/library/nlg_renderer/create_nlg_renderer_from_register_function.h>
#include <alice/nlg/library/nlg_renderer/nlg_renderer.h>

#include <alice/protos/data/contacts.pb.h>
#include <alice/protos/data/fm_radio_info.pb.h>
#include <alice/protos/data/news_provider.pb.h>

#include <apphost/lib/service_testing/service_testing.h>

#include <library/cpp/testing/gmock_in_unittest/gmock.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/tests_data.h>

#include <util/generic/ptr.h>
#include <util/string/cast.h>

#include <memory>
#include <tuple>

namespace google::protobuf {

bool operator== (const Message& lhs, const Message& rhs) {
    NAlice::TMessageDiff diff(lhs, rhs);
    return diff.AreEqual;
}

} // namespace google::protobuf

namespace NAlice {

using namespace testing;

namespace NTestImpl {
using TNiceMockPatcher = std::function<void(NiceMock<TMockContext>&)>;

void PatchNothing(NiceMock<TMockContext>&) {}

std::variant<TRequest, TWalkerResponse> TestPreProcessRequestWithMockSensors(const TSpeechKitRequest& speechKitRequest,
                                                                         const NMegamind::TStackEngineCore& stackEngineCore,
                                                                         NMetrics::ISensors& sensors,
                                                                         TNiceMockPatcher additionalMocks = PatchNothing,
                                                                         THolder<IResponses> responses = nullptr);
} // namespace NTestImpl

bool operator== (const TTypedSemanticFrameRequest& lhs, const TTypedSemanticFrameRequest& rhs) {
    return std::tie(lhs.SemanticFrame, lhs.Utterance, lhs.ProductScenario, lhs.Origin, lhs.DisableVoiceSession, lhs.DisableShouldListen) ==
        std::tie(rhs.SemanticFrame, rhs.Utterance, rhs.ProductScenario, rhs.Origin, rhs.DisableVoiceSession, rhs.DisableShouldListen);
}

namespace {

constexpr auto EXPECTED_MEGAMIND_RENDERED_ERROR = TStringBuf(R"(
{
    "cards":[
        {
            "type":"simple_text",
            "text":"Прошу прощения, что-то сломалось. Спросите попозже, пожалуйста."
        }
    ],
    "meta":[
        {
            "type":"error",
            "error_type":"http",
            "message":"alice.builder: Test error"
        }
    ]
}
)");

constexpr TStringBuf PHRASE_FAKE_MODIFIER = "Таков путь.";

constexpr auto SKR_WITH_ACTIVE_ACTIONS = TStringBuf(R"({
    "request": {
        "event": {
            "type": "text_input"
        },
        "device_state": {
            "active_actions": {
                "semantic_frames": {
                    "__SF_NAME__": {
                        "typed_semantic_frame": {
                            "search_semantic_frame": {}
                        },
                        "analytics": {
                            "product_scenario": "search",
                            "origin": "Scenario",
                            "purpose": "get_factoid"
                        }
                    }
                }
            }
        }
    }
})");

constexpr auto SKR_FAKE_MODIFIER = TStringBuf(R"(
{
    "application": { "timestamp": "1337" },
    "request": {
        "event": {
            "name": "on_card_action",
            "payload": {},
            "type": "server_action"
        },
        "experiments": {
            "debug_response_modifiers": "1"
        },
        "voice_session": true
    }
}
)");

constexpr auto SKR_POSTROLL_MODIFIER_WITH_MEMENTO = TStringBuf(R"(
{
    "application": { "timestamp": "1337" },
    "request": {
        "event": {
            "name": "on_card_action",
            "payload": {},
            "type": "server_action"
        },
        "experiments": {
            "debug_response_modifiers": "1"
        },
        "voice_session": true
    }
}
)");

constexpr auto PROACTIVITY_STORAGE_LAST_POSTROLL_VIEWS = TStringBuf(R"(
{
    "LastPostrollViews": [
        {
            "ItemId": "postroll_id",
            "Analytics": {
                "SuccessConditions": [{
                    "Frame": { "name": "test_response_frame_name" }
                }],
                Info: "item_info"
            },
            "RequestId": "postroll-show-req-id",
            "BaseId": "base_id"
        }
    ]
}
)");

constexpr auto EXPECTED_FAKE_MODIFIER_MM = TStringBuf(R"(
{
    "response": {
        "cards": [{
            "type": "simple_text",
            "text": "Таков путь."
        }, {
            "type": "simple_text",
            "text": "Я всё сказала."
        }]
    },
    "voice_response": {
        "output_speech": {
            "type": "simple",
            "text": "Таков путь. Я всё сказала."
        },
        "should_listen": "false"
    }
}
)");

constexpr auto MEGAMIND_TUNNELLER_RESPONSE = TStringBuf(R"(
{
    "something_key": "something_value",
    "tunneller_raw_response": "ABCDEF",
    "another_key": {
        "inner_class": "inner_value"
    }
}
)");

constexpr auto MEGAMIND_ANALYTICS_INFO_WITHOUT_WINNER_SCENARIO = TStringBuf(R"(
{
    "original_utterance": "utterance",
    "chosen_utterance": "utterance",
    "shown_utterance": "utterance",
    "tunneller_raw_responses": {
        "megamind": {
            "responses": ["ABCDEF"]
        },
        "alice.response": {
            "responses": ["Response tunneller raws"]
        }
    },
    "users_info": {
        "alice.response": {
            "scenario_user_info": {
                "properties": [{
                    "id": "response.user.info",
                    "name": "response_user_info",
                    "human_readable": "some response description",
                    "profile": {}
                }]
            }
        }
    },
    "device_state_actions": {
        "action_id": {
            "directives": [{"name": "my_directive"}],
            "nlu_hint": {
                "instances": [{"phrase": "my_phrase"}]
            }
        }
    },
    "iot_user_info": {
        "colors": [
            {
                "name": "Малиновый",
                "id": "raspberry"
            }
        ]
    },
    "user_profile": {
        "subscriptions": [
            "kinopoisk"
        ],
        "has_yandex_plus": true
    }
}
)");

constexpr auto MEGAMIND_ANALYTICS_INFO_WITHOUT_EMPTY_WINNER_SCENARIO = TStringBuf(R"(
{
    "original_utterance": "utterance",
    "chosen_utterance": "utterance",
    "shown_utterance": "utterance!",
    "users_info": {
        "alice.response": {
            "scenario_user_info": {
                "properties": [{
                    "id": "response.user.info",
                    "name": "response_user_info",
                    "human_readable": "some response description",
                    "profile": {}
                }]
            }
        }
    }
}
)");

constexpr auto MEGAMIND_ANALYTICS_INFO_WITH_WINNER_SCENARIO = TStringBuf(R"(
{
    "original_utterance": "utterance",
    "chosen_utterance": "utterance",
    "shown_utterance": "utterance",
    "tunneller_raw_responses": {
        "megamind": {
            "responses": ["ABCDEF"]
        },
        "alice.response": {
            "responses": ["Response tunneller raws"]
        }
    },
    "analytics_info": {
        "alice.response": {
            "scenario_analytics_info": {
                "intent": "alice.response.intent",
                "objects": [{
                    "id": "response.object.id",
                    "name": "response_some_object",
                    "human_readable": "yet another response description"
                }]
            },
        }
    },
    "users_info": {
        "alice.response": {
            "scenario_user_info": {
                "properties": [{
                    "id": "response.user.info",
                    "name": "response_user_info",
                    "human_readable": "some response description",
                    "profile": {}
                }]
            }
        }
    },
    "winner_scenario": {
        "name": "alice.response"
    }
}
)");

constexpr auto MEGAMIND_ANALYTICS_INFO_WITH_PARENT_PRODUCT_SCENARIO_NAME = TStringBuf(R"(
{
    "original_utterance": "utterance",
    "chosen_utterance": "utterance",
    "shown_utterance": "utterance",
    "tunneller_raw_responses": {
        "megamind": {
            "responses": ["ABCDEF"]
        },
        "alice.response": {
            "responses": ["Response tunneller raws"]
        }
    },
    "analytics_info": {
        "alice.response": {
            "scenario_analytics_info": {
                "intent": "alice.response.intent",
                "objects": [{
                    "id": "response.object.id",
                    "name": "response_some_object",
                    "human_readable": "yet another response description"
                }]
            },
            "parent_product_scenario_name": "parent_product_scenario"
        }
    },
    "users_info": {
        "alice.response": {
            "scenario_user_info": {
                "properties": [{
                    "id": "response.user.info",
                    "name": "response_user_info",
                    "human_readable": "some response description",
                    "profile": {}
                }]
            }
        }
    },
    "winner_scenario": {
        "name": "alice.response"
    },
    "parent_product_scenario_name": "parent_product_scenario"
}
)");

constexpr auto MEGAMIND_ANALYTICS_INFO_WITH_ONLY_TUNNELLER_WINNER_SCENARIO = TStringBuf(R"(
{
    "original_utterance": "utterance",
    "chosen_utterance": "utterance",
    "shown_utterance": "utterance",
    "tunneller_raw_responses": {
        "alice.response": {
            "responses": ["ABCDEF2"]
        }
    },
    "analytics_info": {
        "alice.response": {
            "scenario_analytics_info": {}
        }
    },
    "users_info": {
        "alice.response": {
            "scenario_user_info": {
                "properties": [{
                    "id": "response.user.info",
                    "name": "response_user_info",
                    "human_readable": "some response description",
                    "profile": {}
                }]
            }
        }
    },
    "winner_scenario": {
        "name": "alice.response"
    }
}
)");

constexpr auto MEGAMIND_ANALYTICS_INFO_WITH_EMPTY_WINNER_SCENARIO = TStringBuf(R"(
{
    "original_utterance": "utterance",
    "chosen_utterance": "utterance",
    "shown_utterance": "utterance",
    "analytics_info": {
        "alice.response": {
        }
    },
    "users_info": {
        "alice.response": {
            "scenario_user_info": {
                "properties": [{
                    "id": "response.user.info",
                    "name": "response_user_info",
                    "human_readable": "some response description",
                    "profile": {}
                }]
            }
        }
    },
    "winner_scenario": {
        "name": "alice.response"
    }
}
)");

constexpr auto MEGAMIND_ANALYTICS_INFO_WITH_SCENARIO_TIMINGS = TStringBuf(R"(
{
    "original_utterance": "utterance",
    "chosen_utterance": "utterance",
    "shown_utterance": "utterance",
    "analytics_info": {
        "alice.response": {
            "scenario_analytics_info": {
                "intent": "alice.response.intent",
                "stage_timings": {
                    "timings": {
                        "commit": {
                            "start_timestamp": "1602072000150000",
                            "source_response_durations": {
                                "protocol-commit": "100000"
                            }
                        },
                        "run": {
                            "start_timestamp": "1602072000005000",
                            "source_response_durations": {
                                "protocol-run": "50000"
                            }
                        }
                    }
                }
            },
            "matched_semantic_frames":[]
        }
    },
    "scenario_timings": {
        "alice.response": {
            "timings": {
                "run": {
                    "start_timestamp": "1602072000005000",
                    "source_response_durations": {
                        "protocol-run": "50000"
                    }
                },
                "commit": {
                    "start_timestamp": "1602072000150000",
                    "source_response_durations": {
                        "protocol-commit": "100000"
                    }
                }
            }
        }
    },
    "winner_scenario": {
        "name": "alice.response"
    }
}
)");

const char* SPEECH_KIT_REQUEST_WITH_ON_SUGGEST_EVENT_AND_SESSION = R"(
{
    "header": {
        "request_id": "d34df00d-c135-4227-8cf8-386d7d989237"
    },
    "request": {
        "event": {
            "name": "on_suggest",
            "payload": {
                "caption": "Что ты умеешь?",
                "request_id": "fab1c962-129f-4a9b-9dab-f0726dfb7d21",
                "suggest_block": {
                    "data": null,
                    "form_update": null,
                    "suggest_type": "onboarding__what_can_you_do",
                    "type": "suggest"
                },
                "utterance": "Что ты умеешь?"
            },
            "type": "server_action"
        }
    },
    "session": "%s"
}
)";

const char* SPEECH_KIT_REQUEST_WITH_ON_CARD_ACTION_EVENT_AND_SESSION = R"(
{
    "header": {
        "request_id": "d34df00d-c135-4227-8cf8-386d7d989237"
    },
    "request": {
        "event": {
            "name": "on_card_action",
            "payload": {},
            "type": "server_action"
        }
    },
    "session": "%s"
}
)";

const char* SPEECH_KIT_REQUEST_WITH_ON_EXTERNAL_BUTTON_EVENT_AND_SESSION = R"(
{
    "header": {
        "request_id": "d34df00d-c135-4227-8cf8-386d7d989237"
    },
    "request": {
        "event": {
            "name": "on_external_button",
            "payload": {},
            "type": "server_action"
        }
    },
    "session": "%s"
}
)";

const char* SPEECH_KIT_REQUEST_WITH_PARENT_PRODUCT_SCENARIO_NAME = R"(
{
    "header": {
        "request_id": "d34df00d-c135-4227-8cf8-386d7d989237"
    },
    "request": {
        "event": {
            "name": "on_card_action",
            "payload": {
                "@parent_product_scenario_name": "_name"
            },
            "type": "server_action"
        }
    },
    "session": "%s"
}
)";

const char* SPEECH_KIT_REQUEST_WITH_NONE_EVENT = R"(
{
    "header": {
        "request_id": "d34df00d-c135-4227-8cf8-386d7d989237"
    },
    "request": {
        "event": {
            "name": "none",
            "payload": {
                "@parent_product_scenario_name": "_name"
            },
            "type": "server_action"
        }
    }
}
)";

const char* TEST_SESSION =
    "eJztXOtyG8eVfhXsuPQrIDkYAATAipOCQJGWSFEiKYuiLjXVM9MARpqb58KbilWyVK5syl5lndqt3XJtNpsfqf2xfxTH8sqyrbzC4BXyBPsIe053z"
    "w0XXkRRkRxIKmGm+/Tp06fPOV/3mZl+KKmqTXvENh1DVaUFqdPz+tqydf/2Zl0hW/"
    "WDS732J4trnTl5e3E9Wuvsy6u3Nnqmuyav3FoPjU5DXt1Z3wmXF1dNt71y/9a6awDt6uJGz+jU5NWP1nfM3Z68fWs9CDu7SButLXfk1Y978uoBtm/"
    "J2024nkMeH2E70+hULm+6t+WV7kYv7PzsMtJvB8Dj+vrOWqeC/Ztry1ev77tEvrKK7TpA85G8Ddfm4sb9/"
    "UWk+5m8en2jd+CtjMj+wG1v9u9v9FsoRw34rm6Y+yBvq9OEepBp+bK8vfdJp+e2L/"
    "W2V9oX2+3l9sVPrq+7K9tbe9628nFk2Fak713e6ZgPVq7supbxkbUL+nI0peVoyzf7+"
    "rJ1sGqv7WibrWvros3Npdue5qzdMJaX5NsbyLOz0l5vfyiVJVe7T/UwkBYeSlFADdWnnmVSdm/"
    "QLomsUFpwIssqSx71AzMIqQMlXWIFtCztECui0sIdaaUyZ6xJ98pS6BMnMDOaw7JkOrzNKRg+"
    "lBxiwy8jcR1iqSRAUuKEs4FOHeKbbjAbPDAtCwTWXdumjkFC03Wkw7EyGCax3J7aBx6uv39KWcLId0Ahdx5KPg081wlo0soA6kq9WalVK3WlBU1cU"
    "6dqSPegXIr/GD+Pv4e/rwZPZkHTSeNJ9VEYUpBcZ32ajheFauBGPt5LrElZSlr+ZfAofhF/"
    "HT8fPMYBE8dxQzZ8Nm+75gHxDbzKhuA+oI5KLLPn2GyMd2SYLD+y+ExfjQJTX6IERsoKgOei6Yf7q8Tp8dtl6rYNw+"
    "c3bQuGueb6NjA8oKJwGWSnbJrblnXJCc2QmRFo7ca+h2NwcPCovLK0HkHLEOZBnm01q80qqHC+VpZucnEloFiySC/"
    "gl6tu77rvakQzeZuZelm6SHumA83L0iUHRlopS5uJqkAhaZfBfjDbNS07UCuyrDbqcqHvrMO7ku3umPTucT1XT9Uz2KH+"
    "YGKPrPbMPcIs3sC55ZpepbZNhg0kaafkWd0YZ0n3JpSDiKabm/"
    "pNCt4IvsZKLgdrZEfYDAkpv+pE4Gh2ZgUPpWu6Hvk+"
    "BfNmt6nQxa7uHR6iuNBsfwlAgdmWtAFmukED5q1SFcW5tAeuAmHhKvEfRN54GibpdRYdwJWoUTTvLcIvJzXEabxO/"
    "ID6N0ybuVBCdLTsrHHbIV7f9clmpAUwlAhiDrZLb5nuYLouBxt01zfB75006qQlG/"
    "STiAZjJinV0CYlvt7H8UCRnaoCDAiUDLF8AoeydM03wQ6INZFgkVqmbYKOmVmBWaNpXHV9r5+"
    "ZGbc48HoIvjbTxmaJ6Hop6JXsEnB3gM9myXHtXAlwwZASkR5arx9JRxiiUPLD1OoT4z0cMnpWfnEfTY/"
    "7CL9Ge4ebTp+A8udFc3430f5RkSF3YBFEsaQLkQ4F8UFdpg8YGVhuqPZ8N/"
    "JQApAm6KM5sfJA3TXDvur5dMd0o0DljcXkmoFKLcv0AGrSMroDAZmPpE8cwxJKTxDQNnXf5RAaqDqxLI0HFUELFGkh6oVxw/"
    "gTaTCBrIhJxeXkwoxDVWSn+6QLjjLbBxldHPnRgFdThgHvD/Ff4meDT0Ghf4pflUuDz0CxzwDgXgwex89L8Q+Adz8OflMCqleDXwHlr+Pn/"
    "zAOGF+TTw5AubRFXJzOJJ/"
    "Jh5JNQ8IkgzUTYYsh8atR4Hvsust1NBd8w4S1AegYOvJfe6XGfU0HmXswG2xQWbFBtainmk7XZciasFMJdLMfmnrA6lQuvHTp/"
    "tLu7VtXDjSl3teWWn29uubp1Y2+5qxHurJmafbSPtlqHaw6az7Z2gtuVa9Y20or1LZuRrdhXQx1GI9E38U+"
    "oNzzXaKH5g7EXV4kbCxZ3b6GAsqSBrRqwMI4VAa59WhiqWexx8k2cqxgiaWlS0ipOOlgVruqDfKankXTDlF3DAmC0EdCNFm2VBGezYdG9zzYccB0M"
    "9ZBUsxGqoZsKXQHXME3VOY4wu6ZeyRlZVhMitWnGqZTwfipYgKFCGVJdx0d1iUOH9oCakUzDdz5eFhCLGkh9COKe4SAaBY1Mp3anuuHKmVgm0h2D6"
    "E21PvAPOmK7oFloFbQPuhw86xdkR8YlONGzti6RJH3GCRNTX9q+n+"
    "fpn8PRYMZQuYJ8GXLeDbLwiIfns0awYZAWaHpRKg5kTbgCwaYQr7KIU6wi1Anxhj03cgyVItlDdKZhPk1oWsN1iJuMsAg6vVgmc2NKnEyXfQMg0Yz"
    "kv7v91/+kzQ03HTpgI3UwLRwgOCPZN9yCdvgixUTa3yIPWmqaAJbLdg+JYKoRQ4J7lqYJVGFLBhoROPIA81Q7maF/"
    "pjGee3RWZoupQYuZdIL1XMDkxnJIVvx8VVNliTqgVmP6DgxaOrvwFhG5XQdVSi3KKZOPGHyQq0+3+"
    "moYP0LkqLUW9WKXJ2Ra83mTK1FjRmt2zRmmq2KbDSqdaXeaEjptKma5cKSLF0liYkWlULCdJTZMDPpUwnfiPZAWREoRM2tdtP5H9Iid+"
    "7xSryHajzCGJ+exRifvuvG6NAe+ZsY49O3aozpMN+sMWbam2CMT9+kMcb/Ddu+VyXYov968GVp8HjweWnwhG0Cnw/+cfDFL1/bUI9l/"
    "E4Z8dj9V8F8UcPnaL0nmIfztuxs5Gjbe+GZ7HrChnZIn+Ms/CSGcyrrvzecSflt/GrwuDT4FesHUx0/xK/"
    "iPw+"
    "e4KIOJC0sJQJYQFmiKXRBeonyjmO2IOXczjB3VGSdPWWYncWnEpprsOcl6PiYKMFk3J18q5nAtUyY2qweF54WpjylD5bYH568CGFGuOBsqvllAOtY"
    "INwLsqlkPKlHfBK6/gw3C7YJMg0KK31qi6dJHBKZaVq0i2MXzJBXjs7HRR7LIOY6MG3SozMJFS5C4V6NfFgSS/"
    "0w9IKFuTmyA5boB7O2EczuE3D+"
    "vVmHhnM9Gs7w50nBXKVerdeqrTlhQepV4s3ZrgZRYQbq3Zk9hcVIMdMgDD6FYt4xbiGqqnkDR4WZuqrMyh8UKrr4tNRzTcx4MXm5MDO8E5D7l1k4/"
    "PBC/eIFuX1BkbN/jYsXFAVVAT8XqlCnFKIWliod+D8JfSnZaUS+oFTHCp1yL3BOpGE1IviISi4tWGRKfKHeieRahfDfWpX/"
    "Vmvil42RV1XEb11U0SHSpqgXLGt62rSqZJekSF01Um7VJRSnsYj/"
    "QPRRxRYcffzQqXiUIZ71DdMWwghvhKFkoqai0AY3tj0CDTONJdywFrqzx9fgDgjsIJsVkM0ynQcFIi5mSrPN/"
    "EJtwzaIFOhsaphR1lFmCilVBhYp1Ukg4+wqB6tl++kzqtoMqa06ka0l1JUzj4vLwWah6Hqn3tOm3NhIc6IcQxzQ8/"
    "F5PmNDkai+yLK9YGaqTfZgK+0gQCiFQB05EMf8gFgCCVJ4+nkXzLXEkObDu9IHjS7+vSv9Iv5d/Cz+Lv5m8IQ9KRh8CrD3rDR4NPhN/"
    "A3g3g+l+EUp/hGfI+DDA3yS8MXsz+eQ2S/yi86vANG/wYafDj6DZt/"
    "gw4fBF6X461L8Mn4BDF4l1AXZ31WgqrZaSkVJceqaR2G9BxZ87mjlJj29V3CVSv3m8IoWwSaBlZo8BE6tEfyqymlVwiIhqUwGJ/"
    "7bHbpvDrEmUwSbIliKYGcf108LwQph4C1C2H8CZr0cPAKEelqKvx88xefgiFj8bYXv4eZbRC64hkIseAE4hbA3eDwOy1J28XcpXSn+"
    "isMYPkUHcPs0fvleAdrwzmurT0nYp/65A9ou76fyXuFZIvSbgzMyhE1D97VutoEaxqDq0P3ozi2FMTm/"
    "I+OMtWM2dcYI3XSbNgW5HMhVpyA3MTa8TYyDPdVz9mLXd/haF+yzvmBvdLH/4j+zPdfzsTszaPIyfiZeABOUz3BPB/"
    "99Pfhc7Nd+xKv3C9LkSqNekecuUj9Sdf18kAzfE6WhCt4QvfsIlhP2zSHXMBzpE+"
    "CpOZICTHON3ZRkYgJxFMEqyZ6rXtz5AdcpPE3hKaGuTeFpxPHfIiz9HnOE/PkY7JU4xLwcPIFfwByoeVH666N/"
    "KbEnamw79gozjUDxGK6+LmGKsRT/CaCH3fMdF6LbZ2y39jJ+MVuK/"
    "5jPJ7JikWvkackf4e5b3JmNA79UPMxicrlSRsDgS+jqEXtM+P7BXrKTWwKPOifso8Rh2/"
    "quCiue5H26dx0DR4V+c1jYGEErDkmNIlSl9c20PAW65hCMjjZJAVAfgs16sdeUlTYFxCkgpoBYnwLixCjwFoHxv/"
    "CTHEwTAgx+O3hSTl4oif8XoYd9h/O4BID4DAsGn/PMJPtUJ342Dsn+TTTL5TE5NQDbfwCofQacXsTfT4SyPglUI/"
    "lOjUHTKd4lOTGWEChjNcE7ghSZQBO8EFPWkZ+9aDEumvFqRs+AGi4BqvHTxQSk/WiuTy1vGoj/"
    "fgPxexxw+chf051E49dxkvGBOAtFXdcF8zwm5Mrt2mKniiH3t/Ef4n8txb+L/xku/"
    "yf+Kv73JI6mL9Sx2FUR30qQKHSF9BAZyb5qi09MDgtfbs7X50/"
    "xvuHI15mTXyc8+gwD8Yomzb1iKF6kHP8G5WudalCulJW3frIBfuxPLC2yJ37sz2pP8bF/"
    "RXy0rxxxvADxQ5O98TqhS1b9ho80+KkcpjBev+MPU+CWnhyjMD/mFAVBgRKnjcCk05MXso/XG1kbJCi0YPaPe/mknVLL5FWyhjk/"
    "KRzZMMGXzvn4hlQ7YsSFrsce53AVvyGjwaU9/OwLBO/0gytXD/"
    "jf3YPmZscOPe1+"
    "W1678bF8bXF9u91pL11tf1wG5ud6EkSHhAFYD4YR8THZ3ZCP7m4oz7Za8t2wMivL8INXvKyFP7yMU4i7yHnguLuOuJfKQwzZLGFlpcLa1OfnGVeF8"
    "2nmuAqKY7gOTzpryeWZr1eZuFXGu1nJ8z6RxOPZNpR5Jm91jMiT2d4bN4enOHDjeGs7jwM4cpM25FrHHsYxetTGRGblcSd3HEE9ckhH/"
    "iSM+dxJGI3cQRi1wkEY1TR6lKQhBpXKpLM0GgUW80UWGJCuOfhhKqh3URwrcDn5NFNYAwi/"
    "Rc1eP+QHAIEbtar1aqvM4v1ljHz1chL7xWedyJWyuI32A+XcbcvIe4tzOjz60JDyCeWpyChPq1mrn0Ae5rrQoHkquZTXkqsl45+"
    "KcjK5wO1l8PZTyZXDx3OTC+LGqfWVyaUcnugkms61tStHHDaTw+t8q83rG9eSI2yc9AgbLBSn2DjHnWIzgXF78SZwOUG7Y869KU8+"
    "eicnpeId1RNbdoz2owxN/YR+boIGPJ/QADsCRDd16K5kel24Cf2j+s1WN2N6H57g40/"
    "3mc8FpOqEw31ys5Hj0sjHtcrEM4Jqoyu1fHRUclxwnZZyqRSlaUxat72DRw2lOcGznU5z5oMTUjGnhyic/"
    "RCFM5yFwXMKnsfzozumw3KgEbtTVbxYMCgxNEq7M/PNVnOmpstkhnRb1ZkGaTaVCiGaUqtNOJIxMfVTHMQ49YqpV/"
    "ztvWKsNQ8lq05h1NO81jSvNc1rTfNa07zWNK81zWtN81rTvNY0rzXNa03zWj+pvNbIjukwl1nwyEheQRZ/Zkb/U+abTaUuKw3p8P8Bf4fCUQ==";

constexpr auto GOOD_QUASAR_REQ = TStringBuf(R"(
{
    "application": {
        "app_id": "ru.yandex.quasar"
    },
    "request": {
        "event": {
            "name": "on_card_action",
            "payload": {},
            "type": "server_action"
        }
    }
}
)");


const TString TEST_SCENARIO_NAME = "_scenario_";
const TString TEST_PRODUCT_SCENARIO_NAME = "_product_scenario_";
const TString TEST_UTTERANCE = "_utterance_";
const TString TEST_CALLBACK_NAME = "_callback_";

class TMockGlobalContextTestWrapper {
public:
    TMockGlobalContextTestWrapper()
        : GeoLookup(JoinFsPaths(GetWorkPath(), "geodata6.bin"))
        , Logger(TRTLogger::NullLogger())
        , Rng(69)
        , NlgRenderer_(NNlg::CreateNlgRendererFromRegisterFunction( ::NAlice::NMegamind::NNlg::RegisterAll, Rng))
    {
        EXPECT_CALL(GlobalCtx, Config()).WillRepeatedly(ReturnRef(Config));
        EXPECT_CALL(GlobalCtx, ScenarioConfigRegistry()).WillRepeatedly(ReturnRef(ScenarioConfigRegistry));
        EXPECT_CALL(GlobalCtx, GeobaseLookup()).WillRepeatedly(ReturnRef(GeoLookup));
        EXPECT_CALL(GlobalCtx, GetFactorDomain()).WillRepeatedly(ReturnRef(FactorDomain));
        EXPECT_CALL(GlobalCtx, ServiceSensors()).WillRepeatedly(ReturnRef(Sensors));
        EXPECT_CALL(GlobalCtx, GetNlgRenderer()).WillRepeatedly(ReturnRef(*NlgRenderer_));
    }

    TMockGlobalContext& Get() {
        return GlobalCtx;
    }

    IRng& GetRng() {
        return Rng;
    }

private:
    TMockGlobalContext GlobalCtx;
    TConfig Config;
    TScenarioConfigRegistry ScenarioConfigRegistry;
    NGeobase::TLookup GeoLookup;
    NFactorSlices::TFactorDomain FactorDomain;
    TMockSensors Sensors;
    TRTLogger& Logger;
    TRng Rng;
    NNlg::INlgRendererPtr NlgRenderer_;
};

class TMockWalkerRequestContextTestWrapper {
public:
    explicit TMockWalkerRequestContextTestWrapper(const TTestSpeechKitRequest& speechKitRequest,
                                                  NMegamind::NMementoApi::TRespGetAllObjects mementoObjects = {})
        : Logger(TRTLogger::NullLogger())
        , RequestCtx(GlobalContextWrapper.Get(), NMegamind::TMockInitializer{})
        , SpeechKitRequest(speechKitRequest)
        , Request(CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest))
        , MementoData(std::move(mementoObjects))
        , ClientInfo(TClientInfoProto{})
        , ServiceContext()
        , ItemProxyAdapter(ServiceContext, Logger, GlobalContextWrapper.Get(), false)
    {
        EXPECT_CALL(Ctx, SpeechKitRequest()).WillRepeatedly(Return(SpeechKitRequest));
        EXPECT_CALL(Ctx, Logger()).WillRepeatedly(ReturnRef(Logger));
        EXPECT_CALL(Ctx, Responses()).WillRepeatedly(ReturnRef(Responses));
        EXPECT_CALL(Ctx, ScenarioConfig(_)).WillRepeatedly(ReturnRef(ScenarioInfraConfig));
        EXPECT_CALL(Ctx, StackEngineCore()).WillRepeatedly(ReturnRef(NMegamind::TStackEngineCore::default_instance()));
        EXPECT_CALL(Ctx, MementoData()).WillRepeatedly(ReturnRef(MementoData));
        EXPECT_CALL(Ctx, ClientInfo()).WillRepeatedly(ReturnRef(ClientInfo));
        EXPECT_CALL(Ctx, Sensors()).WillRepeatedly(ReturnRef(GlobalContextWrapper.Get().ServiceSensors()));

        EXPECT_CALL(WalkerRequestCtx, RequestCtx()).WillRepeatedly(ReturnRef(RequestCtx));
        EXPECT_CALL(WalkerRequestCtx, GlobalCtx()).WillRepeatedly(ReturnRef(GlobalContextWrapper.Get()));
        EXPECT_CALL(Const(WalkerRequestCtx), GlobalCtx()).WillRepeatedly(ReturnRef(GlobalContextWrapper.Get()));
        EXPECT_CALL(WalkerRequestCtx, Ctx()).WillRepeatedly(ReturnRef(Ctx));
        EXPECT_CALL(Const(WalkerRequestCtx), Ctx()).WillRepeatedly(ReturnRef(Ctx));
        EXPECT_CALL(WalkerRequestCtx, Rng()).WillRepeatedly(ReturnRef(GlobalContextWrapper.GetRng()));
        EXPECT_CALL(WalkerRequestCtx, PostClassifyState()).WillRepeatedly(ReturnRef(PostClassifyState));
        EXPECT_CALL(WalkerRequestCtx, ItemProxyAdapter()).WillRepeatedly(ReturnRef(ItemProxyAdapter));

        EXPECT_CALL(WalkerRequestCtx, ModifierRequestFactory).WillRepeatedly(ReturnRef(ModifierRequestFactory));
    }

    TMockWalkerRequestContextTestWrapper()
        : TMockWalkerRequestContextTestWrapper(TSpeechKitRequestBuilder{TSpeechKitRequestBuilder::EPredefined::MinimalWithTextEvent}.Build())
    {
    }

    TMockLightWalkerRequestCtx& Get() {
        return WalkerRequestCtx;
    }

    TMockContext& GetCtx() {
        return Ctx;
    }

    const TRequest& GetRequest() {
        return Request;
    }

    TMockGlobalContextTestWrapper& GetGlobalCtxWrapper() {
        return GlobalContextWrapper;
    }

    NMegamind::TMockPostClassifyState& GetPostClassifyState() {
        return PostClassifyState;
    }

private:
    TMockGlobalContextTestWrapper GlobalContextWrapper;
    TRTLogger& Logger;
    NMegamind::TMockInitializer Initializer;
    NMegamind::TMockRequestCtx RequestCtx;
    TTestSpeechKitRequest SpeechKitRequest;
    TRequest Request;
    TMockLightWalkerRequestCtx WalkerRequestCtx;
    TMockContext Ctx;
    TMockResponses Responses;
    TScenarioInfraConfig ScenarioInfraConfig;
    NMegamind::TMementoData MementoData;
    TClientInfo ClientInfo;
    NMegamind::TMockPostClassifyState PostClassifyState;
    NMegamind::NTesting::TMockModifierRequestFactory ModifierRequestFactory;
    NAppHost::NService::TTestContext ServiceContext;
    NMegamind::TItemProxyAdapter ItemProxyAdapter;
};

class TMockProtocolScenario : public TConfigBasedAppHostPureProtocolScenario {
public:
    using TConfigBasedAppHostPureProtocolScenario::TConfigBasedAppHostPureProtocolScenario;

    MOCK_METHOD(TVector<TString>, GetAcceptedFrames, (), (const, override));
    MOCK_METHOD(TStatus, StartRun, (const IContext& ctx,
                                    const NScenarios::TScenarioRunRequest& request,
                                    NMegamind::TItemProxyAdapter& itemProxyAdapter), (const, override));
    MOCK_METHOD(TErrorOr<NScenarios::TScenarioRunResponse>, FinishRun, (const IContext& ctx,
                                                                        NMegamind::TItemProxyAdapter& itemProxyAdapter), (const, override));
    MOCK_METHOD(TErrorOr<NScenarios::TScenarioContinueResponse>, FinishContinue, (const IContext& ctx,
                                                                                  NMegamind::TItemProxyAdapter&), (const, override));
    MOCK_METHOD(TStatus, StartCommit, (const IContext&,
                                       const NScenarios::TScenarioApplyRequest&,
                                       NMegamind::TItemProxyAdapter&), (const, override));
    MOCK_METHOD(TErrorOr<NScenarios::TScenarioCommitResponse>, FinishCommit, (const IContext&,
                                                                              NMegamind::TItemProxyAdapter&), (const, override));
    MOCK_METHOD(TStatus, StartApply, (const IContext&,
                                      const NScenarios::TScenarioApplyRequest&,
                                      NMegamind::TItemProxyAdapter&), (const, override));
    MOCK_METHOD(TErrorOr<NScenarios::TScenarioApplyResponse>, FinishApply, (const IContext&,
                                                                            NMegamind::TItemProxyAdapter&), (const, override));

    const TScenarioConfig& GetConfig() const override {
        return TConfigBasedAppHostPureProtocolScenario::GetConfig();
    }
};

TScenarioWrapperPtr CreateProtocolScenarioWrapper(const TString& name, TMaybe<EDataSourceType> requiredDataSourceType) {
    TScenarioConfig config;
    config.SetName(name);
    if (requiredDataSourceType.Defined()) {
        config.AddDataSources()->SetType(*requiredDataSourceType);
    }
    auto scenario = MakeSimpleShared<TConfigBasedAppHostPureProtocolScenario>(config);
    auto wrapper = MakeIntrusive<NiceMock<TMockScenarioWrapper>>();
    EXPECT_CALL(*wrapper, Accept(_))
        .Times(AtLeast(1))
        .WillRepeatedly(Invoke([scenario](const IScenarioVisitor& visitor) { visitor.Visit(*scenario); }));
    EXPECT_CALL(*wrapper, GetScenario())
        .Times(AtLeast(1))
        .WillRepeatedly(Invoke([scenario]()->const TScenario& { return *scenario; }));
    return wrapper;
}

class TMockScenario : public TScenario {
public:
    TMockScenario(const TString& name, bool needWebSearch)
        : TScenario(name)
        , NeedWebSearch(needWebSearch)
    {
    }

    bool DependsOnWebSearchResult() const override {
        return NeedWebSearch;
    }

    TVector<TString> GetAcceptedFrames() const override {
        return {};
    }

private:
    bool NeedWebSearch;
};

class TMockScenarioRef : public IScenarioRef {
public:
    TMockScenarioRef(const TMockScenario& scenario)
        : Scenario(scenario)
    {
    }

    void Accept(const IScenarioVisitor&) const override {
    }

    const TMockScenario& GetScenario() const override {
        return Scenario;
    }
private:
    const TMockScenario& Scenario;
};

TIntrusivePtr<IScenarioRef> CreateScenarioRef(const TMockScenario& scenario) {
    return MakeIntrusive<TMockScenarioRef>(scenario);
}

class TFakeStorage final : public ISharedFormulasAdapter {
public:
    TBaseCalcerPtr GetSharedFormula(const TStringBuf /* name */) const override {
        return nullptr;
    }
};

TString InputItemName(TString scenario) {
    return "scenario_" + scenario + "_input";
}

void TestClickCallback(const char* speechKitRequestTemplate, const TStringBuf expectedResponseAnalytics = {}) {
    const TString UTTERANCE = "utterance";
    const TMaybe<TStringBuf> SHOWN_UTTERANCE;
    NiceMock<TMockWalkerRequestContextTestWrapper> walkerRequestContextTestWrapper;
    NiceMock<TMockRunWalkerRequestCtx> runWalkerRequestContext;
    NiceMock<TMockContext> context;
    auto requestCtx = NMegamind::TMockRequestCtx::CreateStrict(walkerRequestContextTestWrapper.Get().GlobalCtx());
    auto ahCtx = NAlice::NMegamind::NTesting::TTestAppHostCtx{walkerRequestContextTestWrapper.Get().GlobalCtx()};
    NMegamind::TItemProxyAdapter& itemProxyAdapter = ahCtx.ItemProxyAdapter();
    EXPECT_CALL(runWalkerRequestContext, Ctx()).WillRepeatedly(ReturnRef(context));
    EXPECT_CALL(runWalkerRequestContext, RequestCtx()).WillRepeatedly(ReturnRef(requestCtx));
    EXPECT_CALL(runWalkerRequestContext, ItemProxyAdapter()).WillRepeatedly(ReturnRef(itemProxyAdapter));
    auto logger = CreateNullLogger();
    TMockSensors sensors;
    EXPECT_CALL(context, Sensors()).WillRepeatedly(ReturnRef(sensors));
    TMockResponses responses;
    EXPECT_CALL(context, Responses()).WillRepeatedly(ReturnRef(responses));
    EXPECT_CALL(context, Logger()).WillRepeatedly(ReturnRef(logger));
    const auto rawSpeechKitRequest = Sprintf(speechKitRequestTemplate, TEST_SESSION);
    auto speechKitRequest = TSpeechKitRequestBuilder(TStringBuf(rawSpeechKitRequest)).Build();
    EXPECT_CALL(context, PolyglotUtterance()).WillRepeatedly(Return(UTTERANCE));
    EXPECT_CALL(context, AsrNormalizedUtterance()).WillRepeatedly(Return(SHOWN_UTTERANCE));
    EXPECT_CALL(context, SpeechKitRequest()).WillRepeatedly(Return(speechKitRequest));
    EXPECT_CALL(context, StackEngineCore()).WillRepeatedly(ReturnRef(NMegamind::TStackEngineCore::default_instance()));
    TScenarioResponse scenarioResponse{/* scenarioName= */ {}, /* scenarioSemanticFrames= */ {},
                                       /* scenarioAcceptsAnyUtterance= */ false};
    auto request = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);
    scenarioResponse.ForceBuilder(speechKitRequest, request, NMegamind::TGuidGenerator{});
    TCommonScenarioWalker scenarioWalker(walkerRequestContextTestWrapper.Get().GlobalCtx());
    const auto response = scenarioWalker.RunPreClassifyStage(runWalkerRequestContext);
    UNIT_ASSERT(!response.Scenarios.empty());
    const auto session = response.Scenarios.front().BuilderIfExists()->ToProto().GetResponse().GetSessions().at("");
    UNIT_ASSERT_EQUAL(session, TEST_SESSION);
    NJson::TJsonValue expectedResponseAnalyticsJson = JsonFromString(R"({"original_utterance":"utterance", "chosen_utterance": "utterance", "shown_utterance":"utterance"})");
    if (expectedResponseAnalytics) {
        expectedResponseAnalyticsJson = JsonFromString(expectedResponseAnalytics);
    }
    UNIT_ASSERT_VALUES_EQUAL(expectedResponseAnalyticsJson, response.AnalyticsInfoBuilder.BuildJson());
}

constexpr TStringBuf GET_NEXT_WARM_UP_SPEECHKIT_REQUEST = R"({
    "request": {
        "event": {
            "name": "@@mm_stack_engine_get_next",
            "payload": {
                "@scenario_name": "_scenario_"
            },
            "type": "server_action",
            "is_warmup": true
        }
    }
})";

static constexpr TStringBuf GET_NEXT_RAW_SPEECHKIT_REQUEST = R"({
        "request": {
            "event": {
                "name": "@@mm_stack_engine_get_next",
                "payload": {
                    "@scenario_name": "_scenario_"
                },
                "type": "server_action"
            }
        }
    })";

static constexpr TStringBuf GET_NEXT_RAW_SPEECHKIT_REQUEST_WITH_RECOVERY_CALLBACK = R"({
        "request": {
            "event": {
                "name": "@@mm_stack_engine_get_next",
                "payload": {
                    "@recovery_callback": {
                        "name": "recovery",
                        "payload": {
                            "@scenario_name": "_scenario_",
                            "data": "value"
                        },
                        "ignore_answer": false,
                        "type": "server_action",
                        "is_led_silent": false
                    },
                    "@scenario_name": "_scenario_"
                },
                "type": "server_action"
            }
        }
    })";

static constexpr TStringBuf SPEECHKIT_REQUEST_WITH_CONTACTS = R"({
    "request": {
        "event": {
            "type": "voice_input"
        }
    },
    "contacts": {
        "status": "ok",
        "data": {
            "is_known_uuid": true,
            "contacts": [
                {
                    "lookup_key": "abc",
                    "_id": 43,
                    "account_name": "test@gmail.com",
                    "first_name": "Test",
                    "contact_id": "123",
                    "account_type": "com.google",
                    "display_name": "Test"
                }
            ],
            "phones": [
                {
                    "lookup_key": "abc",
                    "_id": 44,
                    "account_type": "com.google",
                    "phone": "+79121234567",
                    "type": "mobile"
                }
            ]
        }
    }
    })";

static constexpr TStringBuf SPEECHKIT_REQUEST_WITH_ORIGIN = R"({
    "request": {
        "event": {
            "name": "@@mm_semantic_frame",
            "payload": {
                "typed_semantic_frame": {
                    "player_what_is_playing_semantic_frame": {}
                },
                "analytics": {
                    "product_scenario": "music",
                    "purpose": "multiroom_redirect",
                    "origin": "Scenario"
                },
                "origin": {
                    "device_id": "another-device-id",
                    "uuid": "another-uuid"
                }
            },
            "type": "server_action"
        }
    }
})";


static constexpr auto SPEECHKIT_REQUEST_WITH_ASR_WHISPER = TStringBuf(R"(
{
    "application": {
        "app_id": "aliced",
        "device_manufacturer": "Yandex",
        "device_model": "yandexmicro",
        "device_color": "tiffany"
    },
    "header": {
        "request_id": "test_req_id",
        "random_seed": 12345
    },
    "interfaces": {
        "can_recognize_music": true,
        "can_server_action": true,
        "has_bluetooth": true,
        "has_microphone": true,
        "has_music_player": true,
        "has_reliable_speakers": true,
        "has_synchronized_push": true,
        "supports_video_play_directive": true,
        "supports_video_protocol": true
    },
    "request": {
        "experiments": {
            "test_experiment": "1"
        },
        "event": {
            "type": "voice_input",
            "asr_whisper": true,
            "asr_result":[
                    {
                        "endOfPhrase":true,
                        "normalized":"продолжи песню.",
                        "confidence":1,
                        "words":[
                            {
                                "value":"продолжи",
                                "confidence":1
                            },
                            {
                                "value":"песню",
                                "confidence":1
                            }
                        ],
                        "utterance":"продолжи песню.",
                    }
                ],
        },
        "device_state": {
            "sound_level": 33212,
            "multiroom": {
                "multiroom_session_id": "kek_session_id"
            }
        },
        "additional_options": {
            "server_time_ms": 12345
        }
    }
}
)");

std::variant<TRequest, TWalkerResponse>
TestPreProcessRequestWithGetNext(const NMegamind::TStackEngineCore& stackEngineCore) {
    return NTestImpl::TestPreProcessRequest(TSpeechKitRequestBuilder{GET_NEXT_RAW_SPEECHKIT_REQUEST}.Build(), stackEngineCore);
}

TSemanticFrame MakeSearchSemanticFrame(const TString& utterance) {
    TSemanticFrame frame{};
    frame.SetName("personal_assistant.scenarios.search");
    auto& query = *frame.AddSlots();
    query.SetName("query");
    query.SetType("string");
    query.SetValue(utterance);
    query.AddAcceptedTypes("string");
    frame.MutableTypedSemanticFrame()->MutableSearchSemanticFrame()->MutableQuery()->SetStringValue(utterance);
    return frame;
}

TSemanticFrame MakeIoTBroadcastStartSemanticFrame(const TString& pairingToken, const uint32_t& timeoutMs) {
    TSemanticFrame frame{};
    frame.SetName("alice.iot.voice_discovery.start");
    auto& pairingTokenSlot = *frame.AddSlots();
    pairingTokenSlot.SetName("pairing_token");
    pairingTokenSlot.SetType("string");
    pairingTokenSlot.SetValue(pairingToken);
    pairingTokenSlot.AddAcceptedTypes("string");

    auto& timeoutMsSlot = *frame.AddSlots();
    timeoutMsSlot.SetName("timeout_ms");
    timeoutMsSlot.SetType("uint32");
    timeoutMsSlot.SetValue(ToString(timeoutMs));
    timeoutMsSlot.AddAcceptedTypes("uint32");
    frame.MutableTypedSemanticFrame()->MutableIoTBroadcastStartSemanticFrame()->MutablePairingToken()->SetStringValue(pairingToken);
    frame.MutableTypedSemanticFrame()->MutableIoTBroadcastStartSemanticFrame()->MutableTimeoutMs()->SetUInt32Value(timeoutMs);
    return frame;
}

TSemanticFrame MakeIoTBroadcastSuccessSemanticFrame(const TString& devicesID, const TString& productIDs) {
    TSemanticFrame frame{};
    frame.SetName("alice.iot.voice_discovery.success");
    auto& devicesIDSlot = *frame.AddSlots();
    devicesIDSlot.SetName("devices_id");
    devicesIDSlot.SetType("string");
    devicesIDSlot.SetValue(devicesID);
    devicesIDSlot.AddAcceptedTypes("string");

    auto& productIDsSlot = *frame.AddSlots();
    productIDsSlot.SetName("product_ids");
    productIDsSlot.SetType("string");
    productIDsSlot.SetValue(productIDs);
    productIDsSlot.AddAcceptedTypes("string");
    frame.MutableTypedSemanticFrame()->MutableIoTBroadcastSuccessSemanticFrame()->MutableDevicesID()->SetStringValue(devicesID);
    frame.MutableTypedSemanticFrame()->MutableIoTBroadcastSuccessSemanticFrame()->MutableProductIDs()->SetStringValue(productIDs);
    return frame;
}

TSemanticFrame MakeIoTBroadcastFailureSemanticFrame(const uint32_t& timeoutMs, const TString& reason) {
    TSemanticFrame frame{};
    frame.SetName("alice.iot.voice_discovery.failure");
    auto& timeoutMsSlot = *frame.AddSlots();
    timeoutMsSlot.SetName("timeout_ms");
    timeoutMsSlot.SetType("uint32");
    timeoutMsSlot.SetValue(ToString(timeoutMs));
    timeoutMsSlot.AddAcceptedTypes("uint32");

    auto& reasonSlot = *frame.AddSlots();
    reasonSlot.SetName("reason");
    reasonSlot.SetType("string");
    reasonSlot.SetValue(reason);
    reasonSlot.AddAcceptedTypes("string");

    frame.MutableTypedSemanticFrame()->MutableIoTBroadcastFailureSemanticFrame()->MutableTimeoutMs()->SetUInt32Value(timeoutMs);
    frame.MutableTypedSemanticFrame()->MutableIoTBroadcastFailureSemanticFrame()->MutableReason()->SetStringValue(reason);
    return frame;
}

TSemanticFrame MakeIoTDiscoveryStartSemanticFrame(const TString& ssid, const TString& password, const TString& deviceType, const uint32_t& timeoutMs) {
    TSemanticFrame frame{};
    frame.SetName("alice.iot.voice_discovery.start.v2");
    auto& ssidSlot = *frame.AddSlots();
    ssidSlot.SetName("ssid");
    ssidSlot.SetType("string");
    ssidSlot.SetValue(ssid);
    ssidSlot.AddAcceptedTypes("string");

    auto& passwordSlot = *frame.AddSlots();
    passwordSlot.SetName("password");
    passwordSlot.SetType("string");
    passwordSlot.SetValue(password);
    passwordSlot.AddAcceptedTypes("string");

    auto& deviceTypeSlot = *frame.AddSlots();
    deviceTypeSlot.SetName("device_type");
    deviceTypeSlot.SetType("string");
    deviceTypeSlot.SetValue(deviceType);
    deviceTypeSlot.AddAcceptedTypes("string");

    auto& timeoutMsSlot = *frame.AddSlots();
    timeoutMsSlot.SetName("timeout_ms");
    timeoutMsSlot.SetType("uint32");
    timeoutMsSlot.SetValue(ToString(timeoutMs));
    timeoutMsSlot.AddAcceptedTypes("uint32");

    frame.MutableTypedSemanticFrame()->MutableIoTDiscoveryStartSemanticFrame()->MutableSSID()->SetStringValue(ssid);
    frame.MutableTypedSemanticFrame()->MutableIoTDiscoveryStartSemanticFrame()->MutablePassword()->SetStringValue(password);
    frame.MutableTypedSemanticFrame()->MutableIoTDiscoveryStartSemanticFrame()->MutableDeviceType()->SetStringValue(deviceType);
    frame.MutableTypedSemanticFrame()->MutableIoTDiscoveryStartSemanticFrame()->MutableTimeoutMs()->SetUInt32Value(timeoutMs);
    return frame;
}

TSemanticFrame MakeIoTDiscoverySuccessSemanticFrame(const TString& deviceIDs, const TString& productIDs, const TString& deviceType) {
    TSemanticFrame frame{};
    frame.SetName("alice.iot.voice_discovery.success.v2");
    auto& deviceIDsSlot = *frame.AddSlots();
    deviceIDsSlot.SetName("device_ids");
    deviceIDsSlot.SetType("string");
    deviceIDsSlot.SetValue(deviceIDs);
    deviceIDsSlot.AddAcceptedTypes("string");

    auto& productIDsSlot = *frame.AddSlots();
    productIDsSlot.SetName("product_ids");
    productIDsSlot.SetType("string");
    productIDsSlot.SetValue(productIDs);
    productIDsSlot.AddAcceptedTypes("string");

    auto& deviceTypeSlot = *frame.AddSlots();
    deviceTypeSlot.SetName("device_type");
    deviceTypeSlot.SetType("string");
    deviceTypeSlot.SetValue(deviceType);
    deviceTypeSlot.AddAcceptedTypes("string");

    frame.MutableTypedSemanticFrame()->MutableIoTDiscoverySuccessSemanticFrame()->MutableDeviceIDs()->SetStringValue(deviceIDs);
    frame.MutableTypedSemanticFrame()->MutableIoTDiscoverySuccessSemanticFrame()->MutableProductIDs()->SetStringValue(productIDs);
    frame.MutableTypedSemanticFrame()->MutableIoTDiscoverySuccessSemanticFrame()->MutableDeviceType()->SetStringValue(deviceType);
    return frame;
}

TSemanticFrame MakeIoTDiscoveryFailureSemanticFrame(const uint32_t& timeoutMs, const TString& reason, const TString& deviceType) {
    TSemanticFrame frame{};
    frame.SetName("alice.iot.voice_discovery.failure.v2");
    auto& timeoutMsSlot = *frame.AddSlots();
    timeoutMsSlot.SetName("timeout_ms");
    timeoutMsSlot.SetType("uint32");
    timeoutMsSlot.SetValue(ToString(timeoutMs));
    timeoutMsSlot.AddAcceptedTypes("uint32");

    auto& reasonSlot = *frame.AddSlots();
    reasonSlot.SetName("reason");
    reasonSlot.SetType("string");
    reasonSlot.SetValue(reason);
    reasonSlot.AddAcceptedTypes("string");

    auto& deviceTypeSlot = *frame.AddSlots();
    deviceTypeSlot.SetName("device_type");
    deviceTypeSlot.SetType("string");
    deviceTypeSlot.SetValue(deviceType);
    deviceTypeSlot.AddAcceptedTypes("string");

    frame.MutableTypedSemanticFrame()->MutableIoTDiscoveryFailureSemanticFrame()->MutableTimeoutMs()->SetUInt32Value(timeoutMs);
    frame.MutableTypedSemanticFrame()->MutableIoTDiscoveryFailureSemanticFrame()->MutableReason()->SetStringValue(reason);
    frame.MutableTypedSemanticFrame()->MutableIoTDiscoveryFailureSemanticFrame()->MutableDeviceType()->SetStringValue(deviceType);
    return frame;
}

TSemanticFrame MakeMordoviaHomeScreenSemanticFrame(const TString& deviceIDValue) {
    TSemanticFrame frame{};
    frame.SetName("quasar.mordovia.home_screen");
    auto& deviceID = *frame.AddSlots();
    deviceID.SetName("device_id");
    deviceID.SetType("string");
    deviceID.SetValue(deviceIDValue);
    deviceID.AddAcceptedTypes("string");
    frame.MutableTypedSemanticFrame()->MutableMordoviaHomeScreenSemanticFrame()->MutableDeviceID()->SetStringValue(deviceIDValue);
    return frame;
}

TSemanticFrame MakeGetCallerNameSemanticFrame() {
    TSemanticFrame frame{};
    frame.SetName("alice.messenger_call.get_caller_name");
    frame.MutableTypedSemanticFrame()->MutableGetCallerNameSemanticFrame();
    return frame;
}

TSemanticFrame MakeAddAccountSemanticFrame(const TString& encryptedCode, const TString& signature, const TString& encryptedSessionKey) {
    const TString tokenType = "XToken";

    TSemanticFrame frame{};
    frame.SetName("alice.multiaccount.add_account");
    auto& tokenTypeSlot = *frame.AddSlots();
    tokenTypeSlot.SetName("token_type");
    tokenTypeSlot.SetType("enum_value");
    tokenTypeSlot.SetValue(tokenType);
    tokenTypeSlot.AddAcceptedTypes("enum_value");

    auto& codeSlot = *frame.AddSlots();
    codeSlot.SetName("encrypted_code");
    codeSlot.SetType("string");
    codeSlot.SetValue(encryptedCode);
    codeSlot.AddAcceptedTypes("string");

    auto& signatureSlot = *frame.AddSlots();
    signatureSlot.SetName("signature");
    signatureSlot.SetType("string");
    signatureSlot.SetValue(signature);
    signatureSlot.AddAcceptedTypes("string");

    auto& sessionKeySlot = *frame.AddSlots();
    sessionKeySlot.SetName("encrypted_session_key");
    sessionKeySlot.SetType("string");
    sessionKeySlot.SetValue(encryptedSessionKey);
    sessionKeySlot.AddAcceptedTypes("string");

    frame.MutableTypedSemanticFrame()->MutableAddAccountSemanticFrame()->MutableTokenType()->SetEnumValue(TTokenTypeSlot::XToken);
    frame.MutableTypedSemanticFrame()->MutableAddAccountSemanticFrame()->MutableEncryptedCode()->SetStringValue(encryptedCode);
    frame.MutableTypedSemanticFrame()->MutableAddAccountSemanticFrame()->MutableSignature()->SetStringValue(signature);
    frame.MutableTypedSemanticFrame()->MutableAddAccountSemanticFrame()->MutableEncryptedSessionKey()->SetStringValue(encryptedSessionKey);
    return frame;
}

TSemanticFrame MakeRemoveAccountSemanticFrame(const TString& puidValue) {
    TSemanticFrame frame{};
    frame.SetName("alice.multiaccount.remove_account");
    auto& puid = *frame.AddSlots();
    puid.SetName("puid");
    puid.SetType("string");
    puid.SetValue(puidValue);
    puid.AddAcceptedTypes("string");
    frame.MutableTypedSemanticFrame()->MutableRemoveAccountSemanticFrame()->MutablePuid()->SetStringValue(puidValue);
    return frame;
}

TSemanticFrame MakeCentaurCollectCardsSemanticFrame(const TString& carouselIdValue) {
    TSemanticFrame frame{};
    frame.SetName("alice.centaur.collect_cards");
    auto& carouselId = *frame.AddSlots();
    carouselId.SetName("carousel_id");
    carouselId.SetType("string");
    carouselId.SetValue(carouselIdValue);
    carouselId.AddAcceptedTypes("string");
    frame.MutableTypedSemanticFrame()->MutableCentaurCollectCardsSemanticFrame()->MutableCarouselId()->SetStringValue(carouselIdValue);
    return frame;
}

TSemanticFrame MakeCentaurCollectMainScreenSemanticFrame() {
    TSemanticFrame frame{};
    frame.SetName("alice.centaur.collect_main_screen");
    frame.MutableTypedSemanticFrame()->MutableCentaurCollectMainScreenSemanticFrame();
    return frame;
}

TSemanticFrame MakeCentaurCollectUpperShutterSemanticFrame() {
    TSemanticFrame frame{};
    frame.SetName("alice.centaur.collect_upper_shutter");
    frame.MutableTypedSemanticFrame()->MutableCentaurCollectUpperShutterSemanticFrame();
    return frame;
}

TSemanticFrame MakeCentaurGetCardSemanticFrame(const TString& carouselIdValue) {
    TSemanticFrame frame{};
    frame.SetName("alice.centaur.get_card");
    auto& carouselId = *frame.AddSlots();
    carouselId.SetName("carousel_id");
    carouselId.SetType("string");
    carouselId.SetValue(carouselIdValue);
    carouselId.AddAcceptedTypes("string");
    frame.MutableTypedSemanticFrame()->MutableCentaurGetCardSemanticFrame()->MutableCarouselId()->SetStringValue(carouselIdValue);
    return frame;
}

TSemanticFrame MakeGetPhotoFrameSemanticFrame(const TString& carouselIdValue, size_t photoIdValue) {
    TSemanticFrame frame{};
    frame.SetName("alice.scenarios.get_photo_frame");

    auto& carouselId = *frame.AddSlots();
    carouselId.SetName("carousel_id");
    carouselId.SetType("string");
    carouselId.SetValue(carouselIdValue);
    carouselId.AddAcceptedTypes("string");
    frame.MutableTypedSemanticFrame()->MutableGetPhotoFrameSemanticFrame()->MutableCarouselId()->SetStringValue(carouselIdValue);

    auto& photoId = *frame.AddSlots();
    photoId.SetName("photo_id");
    photoId.SetType("num");
    photoId.SetValue(ToString(photoIdValue));
    photoId.AddAcceptedTypes("num");
    frame.MutableTypedSemanticFrame()->MutableGetPhotoFrameSemanticFrame()->MutablePhotoId()->SetNumValue(photoIdValue);

    return frame;
}

TSemanticFrame MakeNewsSemanticFrame(const TString& topicValue, size_t maxCount, bool skipIntro, const NData::TNewsProvider& provider, const TString& where, bool noVoiceButtons, bool goBack) {
    TSemanticFrame frame{};
    frame.SetName("personal_assistant.scenarios.get_news");
    auto& topic = *frame.AddSlots();
    topic.SetName("topic");
    topic.SetType("custom.news_topic");
    topic.SetValue(topicValue);
    topic.AddAcceptedTypes("custom.news_topic");
    auto& maxCountSlot = *frame.AddSlots();
    maxCountSlot.SetName("max_count");
    maxCountSlot.SetType("num");
    maxCountSlot.SetValue(ToString(maxCount));
    maxCountSlot.AddAcceptedTypes("num");
    auto& skipIntroSlot = *frame.AddSlots();
    skipIntroSlot.SetName("skip_intro_and_ending");
    skipIntroSlot.SetType("bool");
    skipIntroSlot.SetValue(ToString(skipIntro));
    skipIntroSlot.AddAcceptedTypes("bool");
    auto& providerSlot = *frame.AddSlots();
    providerSlot.SetName("provider");
    providerSlot.SetType("news_provider");
    providerSlot.SetValue(JsonStringFromProto(provider));
    providerSlot.AddAcceptedTypes("news_provider");
    auto& whereSlot = *frame.AddSlots();
    whereSlot.SetName("where");
    whereSlot.SetType("custom.where");
    whereSlot.SetValue(where);
    whereSlot.AddAcceptedTypes("custom.where");
    auto& noVoiceButtonsSlot = *frame.AddSlots();
    noVoiceButtonsSlot.SetName("disable_voice_buttons");
    noVoiceButtonsSlot.SetType("bool");
    noVoiceButtonsSlot.SetValue(ToString(noVoiceButtons));
    noVoiceButtonsSlot.AddAcceptedTypes("bool");
    auto& goBackSlot = *frame.AddSlots();
    goBackSlot.SetName("go_back");
    goBackSlot.SetType("bool");
    goBackSlot.SetValue(ToString(goBack));
    goBackSlot.AddAcceptedTypes("bool");
    auto& newsFrame = *frame.MutableTypedSemanticFrame()->MutableNewsSemanticFrame();
    newsFrame.MutableTopic()->SetNewsTopicValue(topicValue);
    newsFrame.MutableMaxCount()->SetNumValue(maxCount);
    newsFrame.MutableSkipIntroAndEnding()->SetBoolValue(skipIntro);
    *newsFrame.MutableProvider()->MutableNewsProviderValue() = provider;
    newsFrame.MutableWhere()->SetWhereValue(where);
    newsFrame.MutableDisableVoiceButtons()->SetBoolValue(noVoiceButtons);
    newsFrame.MutableGoBack()->SetBoolValue(goBack);
    return frame;
}

TSemanticFrame MakeMusicPlaySemanticFrame(const TString& specialPlaylistValue, bool disableAutoflow, bool disableNlg, bool playSingleTrack, size_t trackOffsetIndex, const TString& playlist) {
    const TString objectId = "211604";
    const TString objectType = "Album";
    const TString startFromTrackId = "38646012";
    const TString alarmId = "1111ffff-11ff-11ff-11ff-111111ffffff";
    const TString room = "everywhere";
    const TString location = "kitchen";
    const double offsetSec = 2.345;
    const bool disableHistory = true;

    TSemanticFrame frame{};
    frame.SetName("personal_assistant.scenarios.music_play");
    auto& specialPlaylist = *frame.AddSlots();
    specialPlaylist.SetName("special_playlist");
    specialPlaylist.SetType("special_playlist");
    specialPlaylist.SetValue(specialPlaylistValue);
    specialPlaylist.AddAcceptedTypes("special_playlist");

    auto& disableAutoflowSlot = *frame.AddSlots();
    disableAutoflowSlot.SetName("disable_autoflow");
    disableAutoflowSlot.SetType("bool");
    disableAutoflowSlot.SetValue(ToString(disableAutoflow));
    disableAutoflowSlot.AddAcceptedTypes("bool");
    auto& disableNlgSlot = *frame.AddSlots();
    disableNlgSlot.SetName("disable_nlg");
    disableNlgSlot.SetType("bool");
    disableNlgSlot.SetValue(ToString(disableNlg));
    disableNlgSlot.AddAcceptedTypes("bool");
    auto& playSingleTrackSlot = *frame.AddSlots();
    playSingleTrackSlot.SetName("play_single_track");
    playSingleTrackSlot.SetType("bool");
    playSingleTrackSlot.SetValue(ToString(playSingleTrack));
    playSingleTrackSlot.AddAcceptedTypes("bool");
    auto& trackOffsetIndexSlot = *frame.AddSlots();
    trackOffsetIndexSlot.SetName("track_offset_index");
    trackOffsetIndexSlot.SetType("num");
    trackOffsetIndexSlot.SetValue(ToString(trackOffsetIndex));
    trackOffsetIndexSlot.AddAcceptedTypes("num");
    auto& playlistSlot = *frame.AddSlots();
    playlistSlot.SetName("playlist");
    playlistSlot.SetType("string");
    playlistSlot.SetValue(playlist);
    playlistSlot.AddAcceptedTypes("string");
    auto& objectIdSlot = *frame.AddSlots();
    objectIdSlot.SetName("object_id");
    objectIdSlot.SetType("string");
    objectIdSlot.SetValue(objectId);
    objectIdSlot.AddAcceptedTypes("string");
    auto& objectTypeSlot = *frame.AddSlots();
    objectTypeSlot.SetName("object_type");
    objectTypeSlot.SetValue(objectType);
    objectTypeSlot.SetType("enum_value");
    objectTypeSlot.AddAcceptedTypes("enum_value");
    auto& startFromTrackIdSlot = *frame.AddSlots();
    startFromTrackIdSlot.SetName("start_from_track_id");
    startFromTrackIdSlot.SetType("string");
    startFromTrackIdSlot.SetValue(startFromTrackId);
    startFromTrackIdSlot.AddAcceptedTypes("string");
    auto& offsetSecSlot = *frame.AddSlots();
    offsetSecSlot.SetName("offset_sec");
    offsetSecSlot.SetType("double");
    offsetSecSlot.SetValue(ToString(offsetSec));
    offsetSecSlot.AddAcceptedTypes("double");
    auto& alarmIdSlot = *frame.AddSlots();
    alarmIdSlot.SetName("alarm_id");
    alarmIdSlot.SetType("string");
    alarmIdSlot.SetValue(alarmId);
    alarmIdSlot.AddAcceptedTypes("string");
    auto& disableHistorySlot = *frame.AddSlots();
    disableHistorySlot.SetName("disable_history");
    disableHistorySlot.SetType("bool");
    disableHistorySlot.SetValue(ToString(disableHistory));
    disableHistorySlot.AddAcceptedTypes("bool");
    auto& roomSlot = *frame.AddSlots();
    roomSlot.SetName("room");
    roomSlot.SetType("room");
    roomSlot.SetValue(room);
    roomSlot.AddAcceptedTypes("room");
    auto& locationSlot = *frame.AddSlots();
    locationSlot.SetName("location");
    locationSlot.SetType("user.iot.room");
    locationSlot.SetValue(location);
    locationSlot.AddAcceptedTypes("user.iot.room");

    auto& musicPlayFrame = *frame.MutableTypedSemanticFrame()->MutableMusicPlaySemanticFrame();
    musicPlayFrame.MutableSpecialPlaylist()->SetSpecialPlaylistValue(specialPlaylistValue);
    musicPlayFrame.MutableDisableAutoflow()->SetBoolValue(disableAutoflow);
    musicPlayFrame.MutableDisableNlg()->SetBoolValue(disableNlg);
    musicPlayFrame.MutablePlaySingleTrack()->SetBoolValue(playSingleTrack);
    musicPlayFrame.MutableTrackOffsetIndex()->SetNumValue(trackOffsetIndex);
    musicPlayFrame.MutablePlaylist()->SetStringValue(playlist);
    musicPlayFrame.MutableObjectId()->SetStringValue(objectId);
    musicPlayFrame.MutableObjectType()->SetEnumValue(TMusicPlayObjectTypeSlot::Album);
    musicPlayFrame.MutableStartFromTrackId()->SetStringValue(startFromTrackId);
    musicPlayFrame.MutableAlarmId()->SetStringValue(alarmId);
    musicPlayFrame.MutableOffsetSec()->SetDoubleValue(offsetSec);
    musicPlayFrame.MutableDisableHistory()->SetBoolValue(disableHistory);
    musicPlayFrame.MutableRoom()->SetRoomValue(room);
    musicPlayFrame.MutableLocation()->SetUserIotRoomValue(location);
    return frame;
}

TSemanticFrame MakeExternalSkillActivateSemanticFrame(const TString& activationPhraseValue) {
    TSemanticFrame frame{};
    frame.SetName("alice.external_skill_activate");
    auto& activationPhrase = *frame.AddSlots();
    activationPhrase.SetName("activation_phrase");
    activationPhrase.SetType("string");
    activationPhrase.SetValue(activationPhraseValue);
    activationPhrase.AddAcceptedTypes("string");
    frame.MutableTypedSemanticFrame()->MutableExternalSkillActivateSemanticFrame()->MutableActivationPhrase()->SetStringValue(activationPhraseValue);
    return frame;
}

TSemanticFrame MakeExternalSkillFixedActivateSemanticFrame(
    const TString& activationCommandValue,
    const TString& fixedSkillIdValue,
    const TString& payloadValue
) {
    TSemanticFrame frame{};
    frame.SetName("alice.external_skill_fixed_activate");

    auto& fixedSkillId = *frame.AddSlots();
    fixedSkillId.SetName("fixed_skill_id");
    fixedSkillId.SetType("string");
    fixedSkillId.SetValue(fixedSkillIdValue);
    fixedSkillId.AddAcceptedTypes("string");
    frame.MutableTypedSemanticFrame()->MutableExternalSkillFixedActivateSemanticFrame()->MutableFixedSkillId()->SetStringValue(fixedSkillIdValue);

    auto& activationCommand = *frame.AddSlots();
    activationCommand.SetName("activation_command");
    activationCommand.SetType("string");
    activationCommand.SetValue(activationCommandValue);
    activationCommand.AddAcceptedTypes("string");
    frame.MutableTypedSemanticFrame()->MutableExternalSkillFixedActivateSemanticFrame()->MutableActivationCommand()->SetStringValue(activationCommandValue);

    auto& payload = *frame.AddSlots();
    payload.SetName("payload");
    payload.SetType("string");
    payload.SetValue(payloadValue);
    payload.AddAcceptedTypes("string");
    frame.MutableTypedSemanticFrame()->MutableExternalSkillFixedActivateSemanticFrame()->MutablePayload()->SetStringValue(payloadValue);

    return frame;
}

TSemanticFrame MakeVideoSelectionSemanticFrame(const TString& videoAction, uint32_t videoIndex, bool isSilentResponse) {
    TSemanticFrame frame{};
    frame.SetName("personal_assistant.scenarios.quasar.select_video_from_gallery_by_remote_control");

    auto& action = *frame.AddSlots();
    action.SetName("action");
    action.SetType("custom.video_selection_action");
    action.SetValue(videoAction);
    action.AddAcceptedTypes("custom.video_selection_action");
    frame.MutableTypedSemanticFrame()->MutableSelectVideoFromGallerySemanticFrame()->MutableAction()->SetVideoSelectionActionValue(videoAction);

    auto& index = *frame.AddSlots();
    index.SetName("video_index");
    index.SetType("num");
    index.SetValue(ToString(videoIndex));
    index.AddAcceptedTypes("num");
    frame.MutableTypedSemanticFrame()->MutableSelectVideoFromGallerySemanticFrame()->MutableVideoIndex()->SetNumValue(videoIndex);

    auto& silentResponse = *frame.AddSlots();
    silentResponse.SetName("silent_response");
    silentResponse.SetType("bool");
    silentResponse.SetValue(ToString(isSilentResponse));
    silentResponse.AddAcceptedTypes("bool");
    frame.MutableTypedSemanticFrame()->MutableSelectVideoFromGallerySemanticFrame()->MutableSilentResponse()->SetBoolValue(isSilentResponse);

    return frame;
}

TSemanticFrame MakeOpenCurrentVideoSemanticFrame(const TString& videoAction, bool isSilentResponse) {
    TSemanticFrame frame{};
    frame.SetName("personal_assistant.scenarios.quasar.open_current_video");

    auto &action = *frame.AddSlots();
    action.SetName("action");
    action.SetType("custom.video_selection_action");
    action.SetValue(videoAction);
    action.AddAcceptedTypes("custom.video_selection_action");
    frame.MutableTypedSemanticFrame()->MutableOpenCurrentVideoSemanticFrame()->MutableAction()->SetVideoSelectionActionValue(videoAction);

    auto& silentResponse = *frame.AddSlots();
    silentResponse.SetName("silent_response");
    silentResponse.SetType("bool");
    silentResponse.SetValue(ToString(isSilentResponse));
    silentResponse.AddAcceptedTypes("bool");
    frame.MutableTypedSemanticFrame()->MutableOpenCurrentVideoSemanticFrame()->MutableSilentResponse()->SetBoolValue(isSilentResponse);

    return frame;
}

TSemanticFrame MakeOpenCurrentTrailerSemanticFrame(bool isSilentResponse) {
    TSemanticFrame frame{};
    frame.SetName("alice.video.open_current_trailer");

    auto& silentResponse = *frame.AddSlots();
    silentResponse.SetName("silent_response");
    silentResponse.SetType("bool");
    silentResponse.SetValue(ToString(isSilentResponse));
    silentResponse.AddAcceptedTypes("bool");
    frame.MutableTypedSemanticFrame()->MutableOpenCurrentTrailerSemanticFrame()->MutableSilentResponse()->SetBoolValue(isSilentResponse);

    return frame;
}

TSemanticFrame MakeVideoPlayerFinishedSemanticFrame(bool isSilentResponse) {
    TSemanticFrame frame{};
    frame.SetName("alice.quasar.video_player.finished");

    auto& silentResponse = *frame.AddSlots();
    silentResponse.SetName("silent_response");
    silentResponse.SetType("bool");
    silentResponse.SetValue(ToString(isSilentResponse));
    silentResponse.AddAcceptedTypes("bool");
    frame.MutableTypedSemanticFrame()->MutableVideoPlayerFinishedSemanticFrame()->MutableSilentResponse()->SetBoolValue(isSilentResponse);

    return frame;
}

TSemanticFrame MakeSetupRcuStatusSemanticFrame(const TSetupRcuStatusSlot::EValue& statusValue) {
    TSemanticFrame frame{};
    frame.SetName("personal_assistant.scenarios.quasar.setup_rcu.status");

    auto &status = *frame.AddSlots();
    status.SetName("status");
    status.SetValue(TSetupRcuStatusSlot_EValue_Name(statusValue));
    status.SetType("enum_value");
    status.AddAcceptedTypes("enum_value");
    frame.MutableTypedSemanticFrame()->MutableSetupRcuStatusSemanticFrame()->MutableStatus()->SetEnumValue(statusValue);

    return frame;
}

TSemanticFrame MakeSetupRcuAutoStatusSemanticFrame(const TSetupRcuStatusSlot::EValue& statusValue) {
    TSemanticFrame frame{};
    frame.SetName("personal_assistant.scenarios.quasar.setup_rcu_auto.status");

    auto &status = *frame.AddSlots();
    status.SetName("status");
    status.SetValue(TSetupRcuStatusSlot_EValue_Name(statusValue));
    status.SetType("enum_value");
    status.AddAcceptedTypes("enum_value");
    frame.MutableTypedSemanticFrame()->MutableSetupRcuAutoStatusSemanticFrame()->MutableStatus()->SetEnumValue(statusValue);

    return frame;
}

TSemanticFrame MakeSetupRcuCheckStatusSemanticFrame(const TSetupRcuStatusSlot::EValue& statusValue) {
    TSemanticFrame frame{};
    frame.SetName("personal_assistant.scenarios.quasar.setup_rcu_check.status");

    auto &status = *frame.AddSlots();
    status.SetName("status");
    status.SetValue(TSetupRcuStatusSlot_EValue_Name(statusValue));
    status.SetType("enum_value");
    status.AddAcceptedTypes("enum_value");
    frame.MutableTypedSemanticFrame()->MutableSetupRcuCheckStatusSemanticFrame()->MutableStatus()->SetEnumValue(statusValue);

    return frame;
}

TSemanticFrame MakeSetupRcuAdvancedStatusSemanticFrame(const TSetupRcuStatusSlot::EValue& statusValue) {
    TSemanticFrame frame{};
    frame.SetName("personal_assistant.scenarios.quasar.setup_rcu_advanced.status");

    auto &status = *frame.AddSlots();
    status.SetName("status");
    status.SetValue(TSetupRcuStatusSlot_EValue_Name(statusValue));
    status.SetType("enum_value");
    status.AddAcceptedTypes("enum_value");
    frame.MutableTypedSemanticFrame()->MutableSetupRcuAdvancedStatusSemanticFrame()->MutableStatus()->SetEnumValue(statusValue);

    return frame;
}

TSemanticFrame MakeSetupRcuManualStartSemanticFrame() {
    TSemanticFrame frame{};
    frame.SetName("personal_assistant.scenarios.quasar.setup_rcu_manual.start");
    frame.MutableTypedSemanticFrame()->MutableSetupRcuManualStartSemanticFrame();
    return frame;
}

TSemanticFrame MakeSetupRcuAutoStartSemanticFrame(const TString& tvModel) {
    TSemanticFrame frame{};
    frame.SetName("personal_assistant.scenarios.quasar.setup_rcu_auto.start");

    auto &status = *frame.AddSlots();
    status.SetName("tv_model");
    status.SetType("string");
    status.SetValue(tvModel);
    status.AddAcceptedTypes("string");
    frame.MutableTypedSemanticFrame()->MutableSetupRcuAutoStartSemanticFrame()->MutableTvModel()->SetStringValue(tvModel);

    return frame;
}

TSemanticFrame MakeLinkARemoteSemanticFrame(const TString& linkTypeValue) {
    TSemanticFrame frame{};
    frame.SetName("personal_assistant.scenarios.quasar.link_a_remote");

    auto &linkType = *frame.AddSlots();
    linkType.SetName("link_type");
    linkType.SetType("string");
    linkType.SetValue(linkTypeValue);
    linkType.AddAcceptedTypes("string");
    frame.MutableTypedSemanticFrame()->MutableLinkARemoteSemanticFrame()->MutableLinkType()->SetStringValue(linkTypeValue);
    return frame;
}

TSemanticFrame MakeRequestTechnicalSupportSemanticFrame() {
    TSemanticFrame frame{};
    frame.SetName("personal_assistant.scenarios.request_technical_support");
    frame.MutableTypedSemanticFrame()->MutableRequestTechnicalSupportSemanticFrame();
    return frame;
}

TSemanticFrame MakeHardcodedMorningShowSemanticFrame(size_t offset, TStringBuf showType, TStringBuf newsProvider, TStringBuf topic, size_t nextTrackIndex) {
    TSemanticFrame frame{};
    frame.SetName("personal_assistant.scenarios.morning_show");

    auto &offsetSlot = *frame.AddSlots();
    offsetSlot.SetName("offset");
    offsetSlot.SetType("num");
    offsetSlot.SetValue(ToString(offset));
    offsetSlot.AddAcceptedTypes("num");

    auto &showTypeSlot = *frame.AddSlots();
    showTypeSlot.SetName("show_type");
    showTypeSlot.SetType("string");
    showTypeSlot.SetValue(TString{showType});
    showTypeSlot.AddAcceptedTypes("string");

    auto &newsProviderSlot = *frame.AddSlots();
    newsProviderSlot.SetName("news_provider");
    newsProviderSlot.SetType("data");
    newsProviderSlot.SetValue(TString{newsProvider});
    newsProviderSlot.AddAcceptedTypes("data");

    auto &topicSlot = *frame.AddSlots();
    topicSlot.SetName("topic");
    topicSlot.SetType("data");
    topicSlot.SetValue(TString{topic});
    topicSlot.AddAcceptedTypes("data");

    auto &nextTrackIndexSlot = *frame.AddSlots();
    nextTrackIndexSlot.SetName("next_track_index");
    nextTrackIndexSlot.SetType("num");
    nextTrackIndexSlot.SetValue(ToString(nextTrackIndex));
    nextTrackIndexSlot.AddAcceptedTypes("num");

    auto &typedFrame = *frame.MutableTypedSemanticFrame()->MutableHardcodedMorningShowSemanticFrame();
    typedFrame.MutableOffset()->SetNumValue(offset);
    typedFrame.MutableShowType()->SetStringValue(TString{showType});
    typedFrame.MutableNewsProvider()->SetSerializedData(TString{newsProvider});
    typedFrame.MutableTopic()->SetSerializedData(TString{topic});
    typedFrame.MutableNextTrackIndex()->SetNumValue(nextTrackIndex);

    return frame;
}

TSemanticFrame MakeVideoPaymentConfirmedSemanticFrame(bool isSilentResponse) {
    TSemanticFrame frame{};
    frame.SetName("personal_assistant.scenarios.quasar.payment_confirmed");
    frame.MutableTypedSemanticFrame()->MutableVideoPaymentConfirmedSemanticFrame();

    auto& silentResponse = *frame.AddSlots();
    silentResponse.SetName("silent_response");
    silentResponse.SetType("bool");
    silentResponse.SetValue(ToString(isSilentResponse));
    silentResponse.AddAcceptedTypes("bool");
    frame.MutableTypedSemanticFrame()->MutableVideoPaymentConfirmedSemanticFrame()->MutableSilentResponse()->SetBoolValue(isSilentResponse);

    return frame;
}

TSemanticFrame MakeOnboardingStartingCriticalUpdateSemanticFrame(bool isFirstSetup) {
    TSemanticFrame frame{};
    frame.SetName("alice.onboarding.starting_critical_update");

    auto& firstSetup = *frame.AddSlots();
    firstSetup.SetName("is_first_setup");
    firstSetup.SetType("bool");
    firstSetup.SetValue(ToString(isFirstSetup));
    firstSetup.AddAcceptedTypes("bool");
    frame.MutableTypedSemanticFrame()->MutableOnboardingStartingCriticalUpdateSemanticFrame()->MutableIsFirstSetup()->SetBoolValue(isFirstSetup);

    return frame;
}

TSemanticFrame MakeOnboardingStartingConfigureSuccessSemanticFrame() {
    TSemanticFrame frame{};
    frame.SetName("alice.onboarding.starting_configure_success");
    frame.MutableTypedSemanticFrame()->MutableOnboardingStartingConfigureSuccessSemanticFrame();
    return frame;
}

TSemanticFrame MakeRadioPlaySemanticFrame(const TString& fmRadioValue, const TString& fmRadioFreqValue, const NData::TFmRadioInfo& fmRadioInfoValue) {
    TSemanticFrame frame{};
    frame.SetName("personal_assistant.scenarios.radio_play");
    auto& fmRadio = *frame.AddSlots();
    fmRadio.SetName("fm_radio");
    fmRadio.SetType("fm_radio_station");
    fmRadio.SetValue(fmRadioValue);
    fmRadio.AddAcceptedTypes("fm_radio_station");
    frame.MutableTypedSemanticFrame()->MutableRadioPlaySemanticFrame()->MutableFmRadioStation()->SetFmRadioValue(fmRadioValue);

    auto& fmRadioFreq = *frame.AddSlots();
    fmRadioFreq.SetName("fm_radio_freq");
    fmRadioFreq.SetType("fm_radio_freq");
    fmRadioFreq.SetValue(fmRadioFreqValue);
    fmRadioFreq.AddAcceptedTypes("fm_radio_freq");
    frame.MutableTypedSemanticFrame()->MutableRadioPlaySemanticFrame()->MutableFmRadioFreq()->SetFmRadioFreqValue(fmRadioFreqValue);

    auto& fmRadioInfo = *frame.AddSlots();
    fmRadioInfo.SetName("fm_radio_info");
    fmRadioInfo.SetType("fm_radio_info");
    fmRadioInfo.SetValue(JsonStringFromProto(fmRadioInfoValue));
    fmRadioInfo.AddAcceptedTypes("fm_radio_info");
    *frame.MutableTypedSemanticFrame()->MutableRadioPlaySemanticFrame()->MutableFmRadioInfo()->MutableFmRadioInfoValue() = fmRadioInfoValue;

    return frame;
}

TSemanticFrame MakeFmRadioPlaySemanticFrame(const TString& fmRadioValue, const TString& fmRadioFreqValue) {
    TSemanticFrame frame{};
    frame.SetName("alice.music.fm_radio_play");
    auto& fmRadio = *frame.AddSlots();
    fmRadio.SetName("fm_radio");
    fmRadio.SetType("fm_radio_station");
    fmRadio.SetValue(fmRadioValue);
    fmRadio.AddAcceptedTypes("fm_radio_station");
    frame.MutableTypedSemanticFrame()->MutableFmRadioPlaySemanticFrame()->MutableFmRadioStation()->SetFmRadioValue(fmRadioValue);

    auto& fmRadioFreq = *frame.AddSlots();
    fmRadioFreq.SetName("fm_radio_freq");
    fmRadioFreq.SetType("fm_radio_freq");
    fmRadioFreq.SetValue(fmRadioFreqValue);
    fmRadioFreq.AddAcceptedTypes("fm_radio_freq");
    frame.MutableTypedSemanticFrame()->MutableFmRadioPlaySemanticFrame()->MutableFmRadioFreq()->SetFmRadioFreqValue(fmRadioFreqValue);

    return frame;
}

TSemanticFrame MakeSoundLouderSemanticFrame() {
    TSemanticFrame frame{};
    frame.SetName("personal_assistant.scenarios.sound.louder");
    frame.MutableTypedSemanticFrame()->MutableSoundLouderSemanticFrame();
    return frame;
}

TSemanticFrame MakeSoundQuiterSemanticFrame() {
    TSemanticFrame frame{};
    frame.SetName("personal_assistant.scenarios.sound.quiter");
    frame.MutableTypedSemanticFrame()->MutableSoundQuiterSemanticFrame();
    return frame;
}

TSemanticFrame MakeSoundSetLevelSemanticFrameNum(int newLevel) {
    TSemanticFrame frame{};
    frame.SetName("personal_assistant.scenarios.sound.set_level");
    auto& level = *frame.AddSlots();
    level.SetName("level");
    level.SetType("sys.num");
    level.SetValue(ToString(newLevel));
    level.AddAcceptedTypes("sys.num");
    frame.MutableTypedSemanticFrame()->MutableSoundSetLevelSemanticFrame()->MutableLevel()->SetNumLevelValue(newLevel);
    return frame;
}

TSemanticFrame MakeSoundSetLevelSemanticFrameFloat(float newLevel) {
    TSemanticFrame frame{};
    frame.SetName("personal_assistant.scenarios.sound.set_level");
    auto& level = *frame.AddSlots();
    level.SetName("level");
    level.SetType("sys.float");
    level.SetValue(ToString(newLevel));
    level.AddAcceptedTypes("sys.float");
    frame.MutableTypedSemanticFrame()->MutableSoundSetLevelSemanticFrame()->MutableLevel()->SetFloatLevelValue(newLevel);
    return frame;
}

TSemanticFrame MakeGetTimeSemanticFrame() {
    TSemanticFrame frame;
    frame.SetName("personal_assistant.scenarios.get_time");
    auto& where = *frame.AddSlots();
    where.SetName("where");
    where.SetType("special_location");
    where.SetValue("nearest");
    where.AddAcceptedTypes("special_location");
    frame.MutableTypedSemanticFrame()->MutableGetTimeSemanticFrame()->MutableWhere()->SetSpecialLocationValue("nearest");
    return frame;
}

TSemanticFrame MakeIoTScenariosPhraseActionSemanticFrame(const TString& phrase) {
    TSemanticFrame frame{};
    frame.SetName("alice.iot.scenarios.phrase_action");
    auto& phraseSlot = *frame.AddSlots();
    phraseSlot.SetName("phrase");
    phraseSlot.SetType("string");
    phraseSlot.SetValue(phrase);
    phraseSlot.AddAcceptedTypes("string");

    frame.MutableTypedSemanticFrame()->MutableIoTScenariosPhraseActionSemanticFrame()->MutablePhrase()->SetStringValue(phrase);
    return frame;
}

TSemanticFrame MakeIoTScenariosTextActionSemanticFrame(const TString& text) {
    TSemanticFrame frame{};
    frame.SetName("alice.iot.scenarios.text_action");
    auto& textSlot = *frame.AddSlots();
    textSlot.SetName("text");
    textSlot.SetType("string");
    textSlot.SetValue(text);
    textSlot.AddAcceptedTypes("string");

    frame.MutableTypedSemanticFrame()->MutableIoTScenariosTextActionSemanticFrame()->MutableText()->SetStringValue(text);
    return frame;
}

TSemanticFrame MakeIoTScenariosLaunchActionSemanticFrame(const TString& launchID, const uint32_t& stepIndex, const TString& instance, const TString& value) {
    TSemanticFrame frame{};
    frame.SetName("alice.iot.scenarios.launch_action");
    auto& launchIDSlot = *frame.AddSlots();
    launchIDSlot.SetName("launch_id");
    launchIDSlot.SetType("string");
    launchIDSlot.SetValue(launchID);
    launchIDSlot.AddAcceptedTypes("string");

    auto& stepIndexSlot = *frame.AddSlots();
    stepIndexSlot.SetName("step_index");
    stepIndexSlot.SetType("uint32");
    stepIndexSlot.SetValue(ToString(stepIndex));
    stepIndexSlot.AddAcceptedTypes("uint32");

    auto& instanceSlot = *frame.AddSlots();
    instanceSlot.SetName("instance");
    instanceSlot.SetType("string");
    instanceSlot.SetValue(instance);
    instanceSlot.AddAcceptedTypes("string");


    auto& valueSlot = *frame.AddSlots();
    valueSlot.SetName("value");
    valueSlot.SetType("string");
    valueSlot.SetValue(value);
    valueSlot.AddAcceptedTypes("string");

    frame.MutableTypedSemanticFrame()->MutableIoTScenariosLaunchActionSemanticFrame()->MutableLaunchID()->SetStringValue(launchID);
    frame.MutableTypedSemanticFrame()->MutableIoTScenariosLaunchActionSemanticFrame()->MutableStepIndex()->SetUInt32Value(stepIndex);
    frame.MutableTypedSemanticFrame()->MutableIoTScenariosLaunchActionSemanticFrame()->MutableInstance()->SetStringValue(instance);
    frame.MutableTypedSemanticFrame()->MutableIoTScenariosLaunchActionSemanticFrame()->MutableValue()->SetStringValue(value);
    return frame;
}

TSemanticFrame MakeAlarmSetAliceShowSemanticFrame() {
    TSemanticFrame frame{};
    frame.SetName("personal_assistant.scenarios.alarm_set_alice_show");
    frame.MutableTypedSemanticFrame()->MutableAlarmSetAliceShowSemanticFrame();
    return frame;
}

TSemanticFrame MakeTimeCapsuleNextStepSemanticFrame() {
    TSemanticFrame frame{};
    frame.SetName("alice.time_capsule.next_step");
    frame.MutableTypedSemanticFrame()->MutableTimeCapsuleNextStepSemanticFrame();
    return frame;
}

TSemanticFrame MakeTimeCapsuleStopSemanticFrame() {
    TSemanticFrame frame{};
    frame.SetName("alice.time_capsule.stop");
    frame.MutableTypedSemanticFrame()->MutableTimeCapsuleStopSemanticFrame();
    return frame;
}

TSemanticFrame MakeTimeCapsuleStartSemanticFrame() {
    TSemanticFrame frame{};
    frame.SetName("alice.time_capsule.start");
    frame.MutableTypedSemanticFrame()->MutableTimeCapsuleStartSemanticFrame();
    return frame;
}

TSemanticFrame MakeTimeCapsuleResumeSemanticFrame() {
    TSemanticFrame frame{};
    frame.SetName("alice.time_capsule.resume");
    frame.MutableTypedSemanticFrame()->MutableTimeCapsuleResumeSemanticFrame();
    return frame;
}

TSemanticFrame MakeTimeCapsuleSkipQuestionSemanticFrame() {
    TSemanticFrame frame{};
    frame.SetName("alice.time_capsule.skip_question");
    frame.MutableTypedSemanticFrame()->MutableTimeCapsuleSkipQuestionSemanticFrame();
    return frame;
}

void AssertStackEngineEmptyError(const NMegamind::TStackEngineCore& core) {
    auto result = TestPreProcessRequestWithGetNext(core);
    UNIT_ASSERT(std::holds_alternative<TWalkerResponse>(result));
    const auto& response = std::get<TWalkerResponse>(result);
    UNIT_ASSERT(response.Error.Defined());
}

void AssertStackEngineSessionIdError(const NMegamind::TStackEngineCore& core) {
    auto result = TestPreProcessRequestWithGetNext(core);
    UNIT_ASSERT(std::holds_alternative<TWalkerResponse>(result));
    const auto& response = std::get<TWalkerResponse>(result);
    UNIT_ASSERT(response.Scenarios.size() == 1);
    UNIT_ASSERT(response.Scenarios[0].GetScenarioName().empty());
}

void AssertStackEngineParsedUtterance(const NMegamind::TStackEngineCore& core) {
    auto result = TestPreProcessRequestWithGetNext(core);
    UNIT_ASSERT(std::holds_alternative<TRequest>(result));
    const auto& request = std::get<TRequest>(result);
    const auto& semanticFrames = request.GetSemanticFrames();
    UNIT_ASSERT_VALUES_EQUAL(semanticFrames.size(), 1);
    UNIT_ASSERT_MESSAGES_EQUAL(semanticFrames[0], MakeSearchSemanticFrame(TEST_UTTERANCE));
}

NMegamind::TStackEngine::TItem MakeStackEngineItemWithSearchParsedUtterance(const TMaybe<bool> disableOutputSpeech = {},
                                                                            const TMaybe<bool> disableShouldListen = {},
                                                                            bool useDeprecatedParams = false) {
    NMegamind::TStackEngine::TItem item{};
    item.SetScenarioName(TEST_SCENARIO_NAME);
    auto& parsedUtterance = *item.MutableEffect()->MutableParsedUtterance();
    parsedUtterance.SetUtterance(TEST_UTTERANCE);
    auto& typedFrame = *parsedUtterance.MutableTypedSemanticFrame();
    typedFrame.MutableSearchSemanticFrame()->MutableQuery()->SetStringValue(
        TEST_UTTERANCE);
    if (useDeprecatedParams) {
        if (disableOutputSpeech.Defined()) {
            parsedUtterance.MutableParams()->SetDisableOutputSpeech(*disableOutputSpeech);
        }
        if (disableShouldListen.Defined()) {
            parsedUtterance.MutableParams()->SetDisableShouldListen(*disableShouldListen);
        }
    } else {
        if (disableOutputSpeech.Defined()) {
            parsedUtterance.MutableRequestParams()->SetDisableOutputSpeech(*disableOutputSpeech);
        }
        if (disableShouldListen.Defined()) {
            parsedUtterance.MutableRequestParams()->SetDisableShouldListen(*disableShouldListen);
        }
    }
    parsedUtterance.MutableAnalytics()->SetOrigin(TAnalyticsTrackingModule::Scenario);
    parsedUtterance.MutableAnalytics()->SetProductScenario("search");
    parsedUtterance.MutableAnalytics()->SetPurpose("get_factoid");
    return item;
}

void TestPreProcessRequestGetNextWriteMetrics(const NMegamind::TStackEngineCore& stackEngineCore,
                                              TMockSensors& sensors) {
    constexpr auto rawSpeechKitRequest = TStringBuf(R"({
        "request": {
            "event": {
                "name": "@@mm_stack_engine_get_next",
                "payload": {
                    "@scenario_name": "_scenario_"
                },
                "type": "server_action"
            }
        }
    })");
    NTestImpl::TestPreProcessRequestWithMockSensors(TSpeechKitRequestBuilder{rawSpeechKitRequest}.Build(), stackEngineCore,
                                                    sensors);
}

TWizardResponse CreateWizardResponseWithFrame(const TStringBuf frameName, const TStringBuf source) {
    auto begemotResponse = ::NBg::NProto::TAlicePolyglotMergeResponseResult();
    auto& aliceParsedFrames = *begemotResponse.MutableAliceResponse()->MutableAliceParsedFrames();
    aliceParsedFrames.AddFrames()->SetName(TString(frameName));
    aliceParsedFrames.AddSources(TString(source));
    aliceParsedFrames.AddConfidences(1.0);
    return TWizardResponse(std::move(begemotResponse));
}

} // namespace

namespace NTestImpl {

std::variant<TRequest, TWalkerResponse>
TestPreProcessRequestWithMockSensors(const TSpeechKitRequest& speechKitRequest,
                                     const NMegamind::TStackEngineCore& stackEngineCore, NMetrics::ISensors& sensors,
                                     TNiceMockPatcher additionalMocks, THolder<IResponses> responses) {
    TMockGlobalContextTestWrapper globalCtxWrapper;
    TCommonScenarioWalker walker{globalCtxWrapper.Get()};

    NiceMock<TMockResponses> mockResponses;
    IResponses* responsesPtr = responses.Get();
    if (!responsesPtr) {
        responsesPtr = &mockResponses;
    }

    NiceMock<TMockContext> ctx;
    EXPECT_CALL(ctx, Logger()).WillRepeatedly(ReturnRef(TRTLogger::StderrLogger()));
    EXPECT_CALL(ctx, SpeechKitRequest()).WillRepeatedly(Return(speechKitRequest));
    EXPECT_CALL(ctx, StackEngineCore()).WillRepeatedly(ReturnRef(stackEngineCore));
    EXPECT_CALL(ctx, Responses()).WillRepeatedly(ReturnRef( *responsesPtr));
    EXPECT_CALL(ctx, Geobase()).WillRepeatedly(ReturnRef(globalCtxWrapper.Get().GeobaseLookup()));
    EXPECT_CALL(ctx, Sensors()).WillRepeatedly(ReturnRef(sensors));

    NiceMock<TMockRunWalkerRequestCtx> walkerCtx;
    EXPECT_CALL(walkerCtx, Ctx()).WillRepeatedly(ReturnRef(ctx));

    additionalMocks(ctx);

    NMegamind::TMegamindAnalyticsInfoBuilder analyticsInfoBuilder;

    IScenarioWalker::TRunState runState;
    auto requestState = walker.RunPrepareRequest(walkerCtx, runState);
    if (!requestState.Defined()) {
        return std::move(runState.Response);
    }
    return std::move(requestState->Request);
}

std::variant<TRequest, TWalkerResponse> TestPreProcessRequest(const TSpeechKitRequest& speechKitRequest,
                                                          const NMegamind::TStackEngineCore& stackEngineCore) {
    NiceMock<TMockSensors> dummy;
    return TestPreProcessRequestWithMockSensors(speechKitRequest, stackEngineCore, dummy);
}

} // namespace NTestImpl

Y_UNIT_TEST_SUITE(Walker) {

    Y_UNIT_TEST(HasCriticalScenarioVersionMismatch) {
        TScenariosErrors errors;
        UNIT_ASSERT(!NImpl::HasCriticalScenarioVersionMismatch(errors, TRTLogger::StderrLogger()));
        NiceMock<TMockScenario> scenario{/* name= */ "alice.test", /* needWebSearch= */ false};
        errors.Add(scenario.GetName(), /* stage= */ {}, TError{TError::EType::VersionMismatch});
        UNIT_ASSERT(NImpl::HasCriticalScenarioVersionMismatch(errors, TRTLogger::StderrLogger()));
    }

    Y_UNIT_TEST(BuilderTryRenderNlg) {
        StrictMock<TMockWalkerRequestContextTestWrapper> walkerRequestContextTestWrapper;
        TScenarioResponse response{/* scenarioName= */ "alice.builder", /* requestFrames= */ {},
                                   /* scenarioAcceptsAnyUtterance= */ true};
        TResponseBuilderProto proto;
        proto.SetScenarioName(response.GetScenarioName());
        proto.MutableResponse();

        auto skr = TSpeechKitRequestBuilder(TStringBuf(R"({"request":{"event":{"type":"text_input"}}})")).Build();
        auto request = CreateRequest(IEvent::CreateEvent(skr.Event()), skr);
        response.ForceBuilder(skr, request, NMegamind::TGuidGenerator{}, std::move(proto));
        TCommonScenarioWalker scenarioWalker(walkerRequestContextTestWrapper.Get().GlobalCtx());
        const auto error = TError{} << "Test error";
        const auto res = scenarioWalker.TryRenderError(request,
            error, ELanguage::LANG_RUS, response, walkerRequestContextTestWrapper.Get(), TRTLogger::NullLogger());
        UNIT_ASSERT(res);
        {
            TSpeechKitResponseProto::TResponse skResponse;
            const auto status = JsonToProto(NJson::ReadJsonFastTree(EXPECTED_MEGAMIND_RENDERED_ERROR), skResponse);
            UNIT_ASSERT_C(status.ok(), status.ToString());
            UNIT_ASSERT_MESSAGES_EQUAL(skResponse, response.BuilderIfExists()->GetSKRProto().GetResponse());
        }
    }

    Y_UNIT_TEST(TestOnNoWinnerScenarioResponse) {
        TMockScenario scenarioX(/* name= */ "X", /* needWebSearch= */ false);
        TMockScenario scenarioY(/* name= */ "Y", /* needWebSearch= */ false);

        TWalkerResponse walkerResponse;
        walkerResponse.AddScenarioError(scenarioX.GetName(), NImpl::STAGE_INIT, TError{TError::EType::Logic} << "X failed");
        walkerResponse.AddScenarioError(scenarioY.GetName(), NImpl::STAGE_ASK, TError{} << "Y failed");

        auto skr = TSpeechKitRequestBuilder(TStringBuf(R"({
            "request": {
                "event": {
                    "type": "text_input"
                }
            }
        })")).Build();
        auto request = CreateRequest(IEvent::CreateEvent(skr.Event()), skr);

        NiceMock<TMockWalkerRequestContextTestWrapper> walkerRequestContextTestWrapper;
        NiceMock<TMockSensors> sensors;

        EXPECT_CALL(walkerRequestContextTestWrapper.GetCtx(), Sensors()).WillRepeatedly(ReturnRef(sensors));
        EXPECT_CALL(walkerRequestContextTestWrapper.GetCtx(), Language()).WillRepeatedly(Return(ELanguage::LANG_RUS));

        {
            TScenarioResponse topResponse{/* scenarioName= */ "test_scenario",
                                          /* scenarioSemanticFrames= */ {},
                                          /* scenarioAcceptsAnyUtterance= */ true};
            walkerResponse.AddScenarioResponse(std::move(topResponse));
        }

        auto& topResponse = walkerResponse.Scenarios.front();
        const auto& builder = topResponse.ForceBuilder(skr, request,
                                                       /* guidGenerator= */ NMegamind::TGuidGenerator{},
                                                       /* initializedProto= */ [&topResponse]{
            TResponseBuilderProto proto;
            proto.SetScenarioName(topResponse.GetScenarioName());
            proto.MutableResponse();
            return proto;
        }());

        TCommonScenarioWalker scenarioWalker{walkerRequestContextTestWrapper.Get().GlobalCtx()};
        scenarioWalker.OnNoWinnerScenarioResponse(request, walkerResponse, /* scenarioWrappers= */ {},
                                                  /* analyticsInfoBuilder= */ NMegamind::TMegamindAnalyticsInfoBuilder{},
                                                  walkerRequestContextTestWrapper.Get(),
                                                  /* scenariosWithTunnellerResponses= */ {},
                                                  /* error= */ TError{} << "Test error");

        const auto responseJson = SpeechKitResponseToJson(builder.GetSKRProto());
        const auto& actual = responseJson["response"];
        const auto expected = JsonFromString(R"(
        {
            "card": {
                "text": "Прошу прощения, что-то сломалось. Спросите попозже, пожалуйста.",
                "type": "simple_text"
            },
            "cards": [
                {
                    "text": "Прошу прощения, что-то сломалось. Спросите попозже, пожалуйста.",
                    "type": "simple_text"
                }
            ],
            "directives": [],
            "experiments": {},
            "meta": [
                {
                    "error_type": "http",
                    "message": "test_scenario: Test error",
                    "type": "error"
                },
                {
                    "error_type": "http",
                    "message": "{\"scenario_name\":\"Y\",\"message\":\"Y failed\",\"stage\":\"ask\"}",
                    "type": "error"
                },
                {
                    "error_type": "logic",
                    "message": "{\"scenario_name\":\"X\",\"message\":\"X failed\",\"stage\":\"init\"}",
                    "type": "error"
                }
            ]
        })");

        UNIT_ASSERT_VALUES_EQUAL(expected, actual);
    }

    Y_UNIT_TEST(TestOnNoWinnerScenarioResponseWithoutResponseBuilder) {
        TMockScenario scenarioX(/* name= */ "X", /* needWebSearch= */ false);

        TWalkerResponse walkerResponse;
        walkerResponse.AddScenarioError(scenarioX.GetName(), NImpl::STAGE_INIT, TError{TError::EType::Logic} << "X failed");

        auto skr = TSpeechKitRequestBuilder(TStringBuf(R"({
                "request": {
                    "event": {
                        "type": "text_input"
                    }
                }
            })")).Build();
        auto request = CreateRequest(IEvent::CreateEvent(skr.Event()), skr);

        NiceMock<TMockWalkerRequestContextTestWrapper> walkerRequestContextTestWrapper;
        NiceMock<TMockSensors> sensors;

        EXPECT_CALL(walkerRequestContextTestWrapper.GetCtx(), Sensors()).WillRepeatedly(ReturnRef(sensors));
        EXPECT_CALL(walkerRequestContextTestWrapper.GetCtx(), Language()).WillRepeatedly(Return(ELanguage::LANG_RUS));

        {
            TScenarioResponse topResponse{/* scenarioName= */ "test_scenario",
                /* scenarioSemanticFrames= */ {},
                /* scenarioAcceptsAnyUtterance= */ true};
            walkerResponse.AddScenarioResponse(std::move(topResponse));
        }

        auto& topResponse = walkerResponse.Scenarios.front();

        TCommonScenarioWalker scenarioWalker{walkerRequestContextTestWrapper.Get().GlobalCtx()};
        scenarioWalker.OnNoWinnerScenarioResponse(request, walkerResponse, /* scenarioWrappers= */ {},
            /* analyticsInfoBuilder= */ NMegamind::TMegamindAnalyticsInfoBuilder{},
                                                  walkerRequestContextTestWrapper.Get(),
            /* scenariosWithTunnellerResponses= */ {},
            /* error= */ TError{} << "Test error");

        const auto* builder = topResponse.BuilderIfExists();
        UNIT_ASSERT(builder);
        const auto responseJson = SpeechKitResponseToJson(builder->GetSKRProto());
        const auto& actual = responseJson["response"];
        const auto expected = JsonFromString(R"(
            {
                "card": {
                    "text": "Прошу прощения, что-то сломалось. Спросите попозже, пожалуйста.",
                    "type": "simple_text"
                },
                "cards": [
                    {
                        "text": "Прошу прощения, что-то сломалось. Спросите попозже, пожалуйста.",
                        "type": "simple_text"
                    }
                ],
                "directives": [],
                "experiments": {},
                "meta": [
                    {
                        "error_type": "http",
                        "message": "test_scenario: Test error",
                        "type": "error"
                    },
                    {
                        "error_type": "logic",
                        "message": "{\"scenario_name\":\"X\",\"message\":\"X failed\",\"stage\":\"init\"}",
                        "type": "error"
                    }
                ]
            })");

        UNIT_ASSERT_VALUES_EQUAL(expected, actual);
    }

    Y_UNIT_TEST(ApplyResponseModifiers) {
        auto skr = TSpeechKitRequestBuilder{SKR_FAKE_MODIFIER}.Build();
        auto request = CreateRequest(IEvent::CreateEvent(skr.Event()), skr);
        StrictMock<TMockWalkerRequestContextTestWrapper> walkerRequestContextTestWrapper(skr);
        TCommonScenarioWalker scenarioWalker(walkerRequestContextTestWrapper.Get().GlobalCtx());

        NMegamind::TModifiersStorage storage;
        NMegamind::TModifiersInfo info;
        NMegamind::TProactivityLogStorage logStorage;
        const TVector<TSemanticFrame> semanticFramesToMatchPostroll;
        TVector<NMegamind::TModifierPtr> modifiers;
        modifiers.emplace_back(NMegamind::CreateFakeModifier());

        // MM response
        {
            TScenarioResponse response{/* scenarioName= */ "alice.modifier", /* requestFrames= */ {},
                                       /* scenarioAcceptsAnyUtterance= */ true};
            TResponseBuilderProto proto;
            proto.MutableResponse();
            response.ForceBuilder(skr, request, NMegamind::TGuidGenerator{}, std::move(proto))
                .AddSimpleText(TString{PHRASE_FAKE_MODIFIER});
            NMegamind::TMegamindAnalyticsInfoBuilder analyticsInfoBuilder{};
            // apply and check
            scenarioWalker.ApplyResponseModifiers(response, walkerRequestContextTestWrapper.Get(),
                                                  TRTLogger::NullLogger(), storage, info, logStorage,
                                                  /* proactivity= */ {}, semanticFramesToMatchPostroll,
                                                  /* recognizedActionEffectFrames= */ {}, modifiers, analyticsInfoBuilder);
            TSpeechKitResponseProto expected;
            const auto status = JsonToProto(NJson::ReadJsonFastTree(EXPECTED_FAKE_MODIFIER_MM), expected);
            UNIT_ASSERT_C(status.ok(), status.ToString());
            UNIT_ASSERT_MESSAGES_EQUAL(
                response.BuilderIfExists()->GetSKRProto().GetResponse(),
                expected.GetResponse()
            );
            UNIT_ASSERT_MESSAGES_EQUAL(
                response.BuilderIfExists()->GetSKRProto().GetVoiceResponse(),
                expected.GetVoiceResponse()
            );
        }
    }

    Y_UNIT_TEST(ReturnSessionAsIsOnSuggestCallback) {
        TestClickCallback(SPEECH_KIT_REQUEST_WITH_ON_SUGGEST_EVENT_AND_SESSION);
    }

    Y_UNIT_TEST(ReturnSessionAsIsOnCardActionCallback) {
        TestClickCallback(SPEECH_KIT_REQUEST_WITH_ON_CARD_ACTION_EVENT_AND_SESSION);
    }

    Y_UNIT_TEST(ReturnSessionAsIsOnExternalButtonCallback) {
        TestClickCallback(SPEECH_KIT_REQUEST_WITH_ON_EXTERNAL_BUTTON_EVENT_AND_SESSION);
    }

    Y_UNIT_TEST(TestCallbackWithParentProductScenarioName) {
        TestClickCallback(SPEECH_KIT_REQUEST_WITH_PARENT_PRODUCT_SCENARIO_NAME,
                          R"({"shown_utterance":"utterance","original_utterance":"utterance","chosen_utterance": "utterance","parent_product_scenario_name":"_name"})");
    }

    Y_UNIT_TEST(TestPreProcessCallbackNone) {
        auto speechKitRequest = TSpeechKitRequestBuilder(TStringBuf(SPEECH_KIT_REQUEST_WITH_NONE_EVENT)).Build();
        TMockGlobalContextTestWrapper globalCtxWrapper;
        TCommonScenarioWalker walker{globalCtxWrapper.Get()};

        NiceMock<TMockResponses> responses;

        NiceMock<TMockContext> ctx;

        NiceMock<TMockSensors> dummy;

        EXPECT_CALL(ctx, Logger()).WillRepeatedly(ReturnRef(TRTLogger::StderrLogger()));
        EXPECT_CALL(ctx, SpeechKitRequest()).WillRepeatedly(Return(speechKitRequest));
        EXPECT_CALL(ctx, StackEngineCore()).WillRepeatedly(ReturnRef(NMegamind::TStackEngineCore::default_instance()));
        EXPECT_CALL(ctx, Responses()).WillRepeatedly(ReturnRef(responses));
        EXPECT_CALL(ctx, Geobase()).WillRepeatedly(ReturnRef(globalCtxWrapper.Get().GeobaseLookup()));
        EXPECT_CALL(ctx, Sensors()).WillRepeatedly(ReturnRef(dummy));

        NiceMock<TMockRunWalkerRequestCtx> walkerCtx;
        EXPECT_CALL(walkerCtx, Ctx()).WillRepeatedly(ReturnRef(ctx));

        NMegamind::TMegamindAnalyticsInfoBuilder analyticsInfoBuilder;

        IScenarioWalker::TRunState runState;
        auto requestState = walker.RunPrepareRequest(walkerCtx, runState);

        UNIT_ASSERT(requestState.Defined());

        NJson::TJsonValue expectedResponseAnalyticsJson = JsonFromString(R"({"parent_product_scenario_name":"_name"})");

        UNIT_ASSERT_VALUES_EQUAL(expectedResponseAnalyticsJson, runState.AnalyticsInfoBuilder.BuildJson());
    }

    Y_UNIT_TEST(TestWhisperConfigDefaultAfterRunPrepareRequest) {
        auto speechKitRequest = TSpeechKitRequestBuilder(TStringBuf(SPEECHKIT_REQUEST_WITH_ASR_WHISPER)).Build();
        TMockGlobalContextTestWrapper globalCtxWrapper;
        TCommonScenarioWalker walker{globalCtxWrapper.Get()};

        NiceMock<TMockResponses> responses;

        NiceMock<TMockContext> ctx;

        NiceMock<TMockSensors> dummy;

        EXPECT_CALL(ctx, Logger()).WillRepeatedly(ReturnRef(TRTLogger::StderrLogger()));
        EXPECT_CALL(ctx, SpeechKitRequest()).WillRepeatedly(Return(speechKitRequest));
        EXPECT_CALL(ctx, StackEngineCore()).WillRepeatedly(ReturnRef(NMegamind::TStackEngineCore::default_instance()));
        EXPECT_CALL(ctx, Responses()).WillRepeatedly(ReturnRef(responses));
        EXPECT_CALL(ctx, Geobase()).WillRepeatedly(ReturnRef(globalCtxWrapper.Get().GeobaseLookup()));
        EXPECT_CALL(ctx, Sensors()).WillRepeatedly(ReturnRef(dummy));

        NiceMock<TMockRunWalkerRequestCtx> walkerCtx;
        EXPECT_CALL(walkerCtx, Ctx()).WillRepeatedly(ReturnRef(ctx));

        { // without config form memento
            IScenarioWalker::TRunState runState;
            auto requestState = walker.RunPrepareRequest(walkerCtx, runState);

            UNIT_ASSERT(requestState.Defined());

            UNIT_ASSERT(!requestState->Request.GetWhisperInfo()->IsWhisper());
        }

        { // with config from memento
            NMegamind::NMementoApi::TRespGetAllObjects objects;
            auto& whisperConfig = *objects.MutableUserConfigs()->MutableTtsWhisperConfig();
            whisperConfig.SetEnabled(true);
            EXPECT_CALL(ctx, GetTtsWhisperConfig()).WillRepeatedly(ReturnRef(whisperConfig));

            IScenarioWalker::TRunState runState;
            auto requestState = walker.RunPrepareRequest(walkerCtx, runState);

            UNIT_ASSERT(requestState.Defined());

            UNIT_ASSERT(requestState->Request.GetWhisperInfo()->IsWhisper());
        }
    }

    Y_UNIT_TEST(TestPreProcessRequestOnGetNextWithParsedUtterance) {
        NMegamind::TStackEngine stackEngine{};
        stackEngine.Push(MakeStackEngineItemWithSearchParsedUtterance());
        AssertStackEngineParsedUtterance(stackEngine.GetCore());
    }

    Y_UNIT_TEST(TestPreProcessRequestWithEffectOptions) {
        NMegamind::TStackEngine stackEngine{};
        stackEngine.Push(MakeStackEngineItemWithSearchParsedUtterance());
        const auto assertForcedShouldListen = [&stackEngine](const TMaybe<bool>& expectedForcedShouldListen) {
            auto result = TestPreProcessRequestWithGetNext(stackEngine.GetCore());
            UNIT_ASSERT(std::holds_alternative<TRequest>(result));
            const auto& request = std::get<TRequest>(result);
            UNIT_ASSERT_VALUES_EQUAL(request.GetParameters().GetForcedShouldListen(), expectedForcedShouldListen);
        };
        assertForcedShouldListen(/* expectedForcedShouldListen= */ Nothing());
        stackEngine.Push([] {
            auto item = MakeStackEngineItemWithSearchParsedUtterance();
            item.MutableEffect()->MutableOptions()->MutableForcedShouldListen()->set_value(true);
            return item;
        }());
        assertForcedShouldListen(/* expectedForcedShouldListen= */ true);
        stackEngine.Pop();
        stackEngine.Push([] {
            auto item = MakeStackEngineItemWithSearchParsedUtterance();
            item.MutableEffect()->MutableOptions()->MutableForcedShouldListen()->set_value(false);
            return item;
        }());
        assertForcedShouldListen(/* expectedForcedShouldListen= */ false);
    }

    Y_UNIT_TEST(TestPreProcessRequestOnGetNextWithParsedUtteranceAndInvalidEffect) {
        NMegamind::TStackEngine stackEngine{};
        stackEngine.Push(MakeStackEngineItemWithSearchParsedUtterance());
        stackEngine.Push(NMegamind::TStackEngine::TItem{});
        AssertStackEngineParsedUtterance(stackEngine.GetCore());
    }

    Y_UNIT_TEST(TestPreProcessRequestOnGetNextWithCallback) {
        NMegamind::TStackEngine stackEngine{};
        stackEngine.Push([]{
            NMegamind::TStackEngine::TItem item{};
            item.SetScenarioName(TEST_SCENARIO_NAME);
            auto& callback = *item.MutableEffect()->MutableCallback();
            callback.SetName(TEST_CALLBACK_NAME);
            return item;
        }());
        auto result = TestPreProcessRequestWithGetNext(stackEngine.GetCore());
        UNIT_ASSERT(std::holds_alternative<TRequest>(result));
        const auto& request = std::get<TRequest>(result);
        UNIT_ASSERT_VALUES_EQUAL(request.GetSemanticFrames().size(), 0);
        UNIT_ASSERT(request.GetRequestSource() == NScenarios::TScenarioBaseRequest_ERequestSourceType_GetNext);
        const auto* callback = request.GetEvent().AsServerActionEvent();
        UNIT_ASSERT(callback);
        UNIT_ASSERT_STRINGS_EQUAL(callback->GetName(), TEST_CALLBACK_NAME);
        UNIT_ASSERT(NMegamind::TStackEngine{request.GetStackEngineCore()}.IsEmpty());
    }

    Y_UNIT_TEST(TestPreProcessRequestOnGetNextWithInvalidEffect) {
        NMegamind::TStackEngine stackEngine{};
        stackEngine.Push([]{
            NMegamind::TStackEngine::TItem item{};
            item.SetScenarioName(TEST_SCENARIO_NAME);
            return item;
        }());
        AssertStackEngineEmptyError(stackEngine.GetCore());
    }

    Y_UNIT_TEST(TestPreProcessRequestOnGetNextWithInvalidSessionId) {
        NMegamind::TStackEngine stackEngine{};
        stackEngine.StartNewSession("session_id", "product_scenario_name", "scenario_name");
        stackEngine.Push([]{
            NMegamind::TStackEngine::TItem item{};
            item.SetScenarioName(TEST_SCENARIO_NAME);
            return item;
        }());
        AssertStackEngineSessionIdError(stackEngine.GetCore());
    }

    Y_UNIT_TEST(TestPreProcessRequestOnGetNextWithNoEffects) {
        AssertStackEngineEmptyError(/* core= */ {});
    }

    Y_UNIT_TEST(TestPreProcessRequestOnGetNextWithRecoveryCallback) {
        StrictMock<TMockSensors> sensors;
        EXPECT_CALL(sensors, IncRate(NMonitoring::TLabels{
            {"scenario_name", TEST_SCENARIO_NAME},
            {"name", "stack_engine.errors_per_second"},
            {"error_type", "recovered"}
        }));
        EXPECT_CALL(sensors, IncRate(NMonitoring::TLabels{
            {"scenario_name", TEST_SCENARIO_NAME},
            {"name", "stack_engine.recoveries_per_second"},
            {"recovery_type", "from_callback"}
        }));
        EXPECT_CALL(sensors, IncRate(NMonitoring::TLabels{
            {"scenario_name", TEST_SCENARIO_NAME},
            {"name", "stack_engine.errors_per_second"},
            {"error_type", "empty"}
        }));
        auto result = NTestImpl::TestPreProcessRequestWithMockSensors(
            TSpeechKitRequestBuilder{GET_NEXT_RAW_SPEECHKIT_REQUEST_WITH_RECOVERY_CALLBACK}.Build(),
            /* core= */ {},
            sensors);
        UNIT_ASSERT(std::holds_alternative<TRequest>(result));
        const auto& request = std::get<TRequest>(result);
        const auto* callback = request.GetEvent().AsServerActionEvent();
        UNIT_ASSERT(callback);
        UNIT_ASSERT_VALUES_EQUAL(callback->GetName(), "recovery");
        const auto* scenarioName = MapFindPtr(callback->GetPayload().fields(), "@scenario_name");
        UNIT_ASSERT(scenarioName);
        UNIT_ASSERT_VALUES_EQUAL(scenarioName->string_value(), TEST_SCENARIO_NAME);
        const auto* data = MapFindPtr(callback->GetPayload().fields(), "data");
        UNIT_ASSERT(data);
        UNIT_ASSERT_VALUES_EQUAL(data->string_value(), "value");
    }

    Y_UNIT_TEST(TestPreProcessRequestOnGetNextWarmUp) {
        NiceMock<TMockSensors> sensors;
        NMegamind::TStackEngine stackEngine{};
        stackEngine.Push(MakeStackEngineItemWithSearchParsedUtterance());
        auto result = NTestImpl::TestPreProcessRequestWithMockSensors(
            TSpeechKitRequestBuilder{GET_NEXT_WARM_UP_SPEECHKIT_REQUEST}.Build(), stackEngine.GetCore(), sensors);
        UNIT_ASSERT(std::holds_alternative<TRequest>(result));
        const auto& request = std::get<TRequest>(result);
        UNIT_ASSERT(request.IsWarmUp());
    }

    Y_UNIT_TEST(TestPreProcessRequestOnGetNextNoWarmUp) {
        NiceMock<TMockSensors> sensors;
        NMegamind::TStackEngine stackEngine{};
        stackEngine.Push(MakeStackEngineItemWithSearchParsedUtterance());
        auto result = NTestImpl::TestPreProcessRequestWithMockSensors(
            TSpeechKitRequestBuilder{GET_NEXT_RAW_SPEECHKIT_REQUEST}.Build(), stackEngine.GetCore(), sensors);
        UNIT_ASSERT(std::holds_alternative<TRequest>(result));
        const auto& request = std::get<TRequest>(result);
        UNIT_ASSERT(!request.IsWarmUp());
    }

    Y_UNIT_TEST(TestPreProcessRequestOnFrameWithDisableVoiceSession) {
        NiceMock<TMockSensors> sensors;
        NMegamind::TStackEngine stackEngine{};
        stackEngine.Push(MakeStackEngineItemWithSearchParsedUtterance(/* disableOutputSpeech= */ true));
        auto result = NTestImpl::TestPreProcessRequestWithMockSensors(
            TSpeechKitRequestBuilder{GET_NEXT_RAW_SPEECHKIT_REQUEST}.Build(), stackEngine.GetCore(), sensors);
        UNIT_ASSERT(std::holds_alternative<TRequest>(result));
        const auto& request = std::get<TRequest>(result);
        UNIT_ASSERT(request.GetDisableVoiceSession());
        UNIT_ASSERT(!request.GetDisableShouldListen());
    }

    Y_UNIT_TEST(TestPreProcessRequestOnFrameWithDisableShouldListen) {
        NiceMock<TMockSensors> sensors;
        NMegamind::TStackEngine stackEngine{};
        stackEngine.Push(MakeStackEngineItemWithSearchParsedUtterance(/* disableOutputSpeech= */ false, /* disableShouldListen= */ true));
        auto result = NTestImpl::TestPreProcessRequestWithMockSensors(
            TSpeechKitRequestBuilder{GET_NEXT_RAW_SPEECHKIT_REQUEST}.Build(), stackEngine.GetCore(), sensors);
        UNIT_ASSERT(std::holds_alternative<TRequest>(result));
        const auto& request = std::get<TRequest>(result);
        UNIT_ASSERT(!request.GetDisableVoiceSession());
        UNIT_ASSERT(request.GetDisableShouldListen());
    }

    Y_UNIT_TEST(TestPreProcessRequestOnFrameWithDisableVoiceSessionDeprecated) {
        NiceMock<TMockSensors> sensors;
        NMegamind::TStackEngine stackEngine{};
        stackEngine.Push(MakeStackEngineItemWithSearchParsedUtterance(/* disableOutputSpeech= */ true, /* disableShouldListen= */ false, /* useDeprecatedParams= */ true));
        auto result = NTestImpl::TestPreProcessRequestWithMockSensors(
            TSpeechKitRequestBuilder{GET_NEXT_RAW_SPEECHKIT_REQUEST}.Build(), stackEngine.GetCore(), sensors);
        UNIT_ASSERT(std::holds_alternative<TRequest>(result));
        const auto& request = std::get<TRequest>(result);
        UNIT_ASSERT(request.GetDisableVoiceSession());
        UNIT_ASSERT(!request.GetDisableShouldListen());
    }

    Y_UNIT_TEST(TestPreProcessRequestOnFrameWithDisableShouldListenDeprecated) {
        NiceMock<TMockSensors> sensors;
        NMegamind::TStackEngine stackEngine{};
        stackEngine.Push(MakeStackEngineItemWithSearchParsedUtterance(/* disableOutputSpeech= */ false, /* disableShouldListen= */ true, /* useDeprecatedParams= */ true));
        auto result = NTestImpl::TestPreProcessRequestWithMockSensors(
            TSpeechKitRequestBuilder{GET_NEXT_RAW_SPEECHKIT_REQUEST}.Build(), stackEngine.GetCore(), sensors);
        UNIT_ASSERT(std::holds_alternative<TRequest>(result));
        const auto& request = std::get<TRequest>(result);
        UNIT_ASSERT(!request.GetDisableVoiceSession());
        UNIT_ASSERT(request.GetDisableShouldListen());
    }

    Y_UNIT_TEST(TestPreProcessRequestOnGetNextWithRecoveryCallbackWarmUp) {
        StrictMock<TMockSensors> sensors;
        EXPECT_CALL(sensors, IncRate(NMonitoring::TLabels{
            {"scenario_name", TEST_SCENARIO_NAME},
            {"name", "stack_engine.errors_per_second"},
            {"error_type", "invalid_session"}
        }));
        EXPECT_CALL(sensors, IncRate(NMonitoring::TLabels{
            {"scenario_name", TEST_SCENARIO_NAME},
            {"name", "stack_engine.errors_per_second"},
            {"error_type", "empty"}
        }));
        EXPECT_CALL(sensors, IncRate(NMonitoring::TLabels{
            {"scenario_name", TEST_SCENARIO_NAME},
            {"name", "stack_engine.errors_per_second"},
            {"error_type", "recovered"}
        }));
        EXPECT_CALL(sensors, IncRate(NMonitoring::TLabels{
            {"scenario_name", TEST_SCENARIO_NAME},
            {"name", "stack_engine.recoveries_per_second"},
            {"recovery_type", "from_callback"}
        }));

        auto requestJson = JsonFromString(GET_NEXT_RAW_SPEECHKIT_REQUEST_WITH_RECOVERY_CALLBACK);
        requestJson["request"]["event"]["is_warmup"] = true;

        NMegamind::TStackEngine stackEngine{};
        stackEngine.StartNewSession("session_id", "product_scenario_name", "scenario_name");

        auto result = NTestImpl::TestPreProcessRequestWithMockSensors(
            TSpeechKitRequestBuilder{requestJson}.Build(),
            stackEngine.GetCore(),
            sensors);

        UNIT_ASSERT(std::holds_alternative<TRequest>(result));
        const auto& request = std::get<TRequest>(result);
        const auto* callback = request.GetEvent().AsServerActionEvent();
        UNIT_ASSERT(callback);
        UNIT_ASSERT_VALUES_EQUAL(callback->GetName(), "recovery");
        const auto* scenarioName = MapFindPtr(callback->GetPayload().fields(), "@scenario_name");
        UNIT_ASSERT(scenarioName);
        UNIT_ASSERT_VALUES_EQUAL(scenarioName->string_value(), TEST_SCENARIO_NAME);
        const auto* data = MapFindPtr(callback->GetPayload().fields(), "data");
        UNIT_ASSERT(data);
        UNIT_ASSERT_VALUES_EQUAL(data->string_value(), "value");
    }

    Y_UNIT_TEST(TestPreProcessRequestOnGetNextWithRecoveryFromMemento) {
        NiceMock<TMockSensors> sensors;
        EXPECT_CALL(sensors, IncRate(_)).Times(AnyNumber());
        EXPECT_CALL(sensors, AddHistogram(_, _, _)).Times(AnyNumber());
        EXPECT_CALL(sensors, IncRate(NMonitoring::TLabels{
            {"scenario_name", TEST_SCENARIO_NAME},
            {"name", "stack_engine.recoveries_per_second"},
            {"recovery_type", "from_memento"}
        })).Times(1);

        NMegamind::TStackEngine stackEngine{};
        stackEngine.Push(MakeStackEngineItemWithSearchParsedUtterance());

        NMegamind::NMementoApi::TRespGetAllObjects objects;
        auto& mementoScenarioData = *objects.MutableScenarioData();
        mementoScenarioData[TString{MM_STACK_ENGINE_MEMENTO_KEY}].PackFrom(stackEngine.GetCore());
        NMegamind::TMementoData mementoData{std::move(objects)};

        auto result = NTestImpl::TestPreProcessRequestWithMockSensors(
            TSpeechKitRequestBuilder{GET_NEXT_RAW_SPEECHKIT_REQUEST}.Build(),
            /* core= */ {},
            sensors,
            /* additionalMocks= */ [&mementoData](NiceMock<TMockContext>& ctx) {
                EXPECT_CALL(ctx, HasExpFlag(EXP_ENABLE_STACK_ENGINE_MEMENTO_BACKUP)).WillOnce(Return(true));
                EXPECT_CALL(ctx, HasExpFlag(EXP_DISABLE_STACK_ENGINE_RECOVERY_CALLBACK)).WillRepeatedly(Return(false));
                EXPECT_CALL(ctx, HasExpFlag(EXP_PASS_ALL_PARSED_SEMANTIC_FRAMES)).WillRepeatedly(Return(false));

                EXPECT_CALL(ctx, MementoData()).WillRepeatedly(ReturnRef(mementoData));
            });
        UNIT_ASSERT(std::holds_alternative<TRequest>(result));
        const auto& request = std::get<TRequest>(result);
        const auto& semanticFrames = request.GetSemanticFrames();
        UNIT_ASSERT_VALUES_EQUAL(semanticFrames.size(), 1);
        UNIT_ASSERT_MESSAGES_EQUAL(semanticFrames[0], MakeSearchSemanticFrame(TEST_UTTERANCE));
    }

    Y_UNIT_TEST(TestPreProcessRequestWithActiveActions) {
        THolder<IResponses> responses = [] {
            auto responses = MakeHolder<TMockResponses>();
            responses->SetWizardResponse(CreateWizardResponseWithFrame("__SF_NAME__", "AliceActionRecognizer"));
            return responses;
        }();
        const auto skr = TSpeechKitRequestBuilder{SKR_WITH_ACTIVE_ACTIONS}.Build();
        NiceMock<TMockSensors> dummy;
        const auto result = NTestImpl::TestPreProcessRequestWithMockSensors(
            skr, /* stackEngineCore= */ {}, dummy, NTestImpl::PatchNothing, std::move(responses));
        const auto* request = std::get_if<TRequest>(&result);
        UNIT_ASSERT(request);
        UNIT_ASSERT_VALUES_EQUAL(request->GetSemanticFrames().size(), 1);
        UNIT_ASSERT(request->GetSemanticFrames().front().GetTypedSemanticFrame().HasSearchSemanticFrame());
    }

    Y_UNIT_TEST(TestPreProcessRequestWithContacts) {
        NAlice::NData::TContactsList contacts;
        contacts.SetIsKnownUuid(true);

        auto& contact = *contacts.AddContacts();
        contact.SetId(43);
        contact.SetLookupKey("abc");
        contact.SetAccountName("test@gmail.com");
        contact.SetFirstName("Test");
        contact.SetContactId(123);
        contact.SetAccountType("com.google");
        contact.SetDisplayName("Test");

        auto& phone = *contacts.AddPhones();
        phone.SetId(44);
        phone.SetLookupKey("abc");
        phone.SetAccountType("com.google");
        phone.SetPhone("+79121234567");
        phone.SetType("mobile");

        const auto skr = TSpeechKitRequestBuilder{SPEECHKIT_REQUEST_WITH_CONTACTS}.Build();
        const auto result = NTestImpl::TestPreProcessRequest(skr, /* stackEngineCore= */ {});
        const auto* request = std::get_if<TRequest>(&result);
        UNIT_ASSERT(request);
        UNIT_ASSERT(request->GetContactsList().Defined());
        UNIT_ASSERT_MESSAGES_EQUAL(*request->GetContactsList(), contacts);
    }

    Y_UNIT_TEST(TestPreProcessRequestWithOrigin) {
        TOrigin origin;
        origin.SetDeviceId("another-device-id");
        origin.SetUuid("another-uuid");

        const auto skr = TSpeechKitRequestBuilder{SPEECHKIT_REQUEST_WITH_ORIGIN}.Build();
        const auto result = NTestImpl::TestPreProcessRequest(skr, /* stackEngineCore= */ {});
        const auto* request = std::get_if<TRequest>(&result);
        UNIT_ASSERT(request);
        UNIT_ASSERT(request->GetOrigin().Defined());
        UNIT_ASSERT_MESSAGES_EQUAL(*request->GetOrigin(), origin);
    }

    Y_UNIT_TEST(TypedSemanticFrameRequest) {
        const auto event = JsonToProto<TEvent>(JsonFromString(TStringBuf(R"({
            "payload": {
                "typed_semantic_frame": {
                    "search_semantic_frame": {
                        "query": {
                            "string_value": "что такое ананас"
                        }
                    }
                },
                "utterance": "что такое ананас",
                "analytics": {
                    "product_scenario": "search",
                    "origin": "Scenario",
                    "purpose": "get_factoid"
                }
            },
            "name": "@@mm_semantic_frame",
            "type": "server_action"
        })")));
        const auto frame = TTypedSemanticFrameRequest{event.GetPayload()};
        UNIT_ASSERT_STRINGS_EQUAL(frame.Utterance, TStringBuf("что такое ананас"));
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeSearchSemanticFrame("что такое ананас"));
        UNIT_ASSERT_STRINGS_EQUAL(frame.ProductScenario, "search");
    }

    Y_UNIT_TEST(TypedSemanticFrameRequestWithoutUtterance) {
        const auto event = JsonToProto<TEvent>(JsonFromString(TStringBuf(R"({
            "payload": {
                "typed_semantic_frame": {
                    "search_semantic_frame": {
                        "query": {
                            "string_value": ""
                        }
                    }
                },
                "analytics": {
                    "product_scenario": "search",
                    "origin": "Scenario",
                    "purpose": "get_factoid"
                }
            },
            "name": "@@mm_semantic_frame",
            "type": "server_action"
        })")));
        const auto frame = TTypedSemanticFrameRequest{event.GetPayload()};
        UNIT_ASSERT_STRINGS_EQUAL(frame.Utterance, Default<TStringBuf>());
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeSearchSemanticFrame(""));
    }

    Y_UNIT_TEST(TypedSemanticFrameIoTBroadcastStartRequest) {
        const auto eventPayload = JsonToProto<google::protobuf::Struct>(JsonFromString(TStringBuf(R"({
                "typed_semantic_frame": {
                    "iot_broadcast_start": {
                        "pairing_token": {
                            "string_value": "token"
                        },
                        "timeout_ms": {
                            "uint32_value": 30
                        }
                    }
                },
                "utterance": "",
                "analytics": {
                    "product_scenario": "IoTVoiceDiscovery",
                    "origin": "Scenario",
                    "purpose": "notify_about_broadcast_start"
                }
            })")));
        const auto frame = TTypedSemanticFrameRequest{eventPayload};
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeIoTBroadcastStartSemanticFrame("token", 30));
    }


    Y_UNIT_TEST(TypedSemanticFrameIoTBroadcastSuccessRequest) {
        const auto eventPayload = JsonToProto<google::protobuf::Struct>(JsonFromString(TStringBuf(R"({
                "typed_semantic_frame": {
                    "iot_broadcast_success": {
                        "devices_id": {
                            "string_value": "device_id"
                        },
                        "product_ids": {
                            "string_value": "product_id"
                        }
                    }
                },
                "utterance": "",
                "analytics": {
                    "product_scenario": "IoTVoiceDiscovery",
                    "origin": "Scenario",
                    "purpose": "notify_about_broadcast_success"
                }
            })")));
        const auto frame = TTypedSemanticFrameRequest{eventPayload};
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeIoTBroadcastSuccessSemanticFrame("device_id", "product_id"));
    }

    Y_UNIT_TEST(TypedSemanticFrameIoTBroadcastFailureRequest) {
        const auto eventPayload = JsonToProto<google::protobuf::Struct>(JsonFromString(TStringBuf(R"({
                "typed_semantic_frame": {
                    "iot_broadcast_failure": {
                        "timeout_ms": {
                            "uint32_value": 30
                        },
                        "reason": {
                            "string_value": "some_reason"
                        }
                    }
                },
                "utterance": "",
                "analytics": {
                    "product_scenario": "IoTVoiceDiscovery",
                    "origin": "Scenario",
                    "purpose": "notify_about_broadcast_failure"
                }
            })")));
        const auto frame = TTypedSemanticFrameRequest{eventPayload};
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeIoTBroadcastFailureSemanticFrame(30, "some_reason"));
    }

    Y_UNIT_TEST(TypedSemanticFrameIoTDiscoveryStartRequest) {
        const auto eventPayload = JsonToProto<google::protobuf::Struct>(JsonFromString(TStringBuf(R"({
                "typed_semantic_frame": {
                    "iot_discovery_start": {
                        "ssid": {
                            "string_value": "ssid"
                        },
                        "password": {
                            "string_value": "password"
                        },
                        "device_type": {
                            "string_value": "device_type"
                        },
                        "timeout_ms": {
                            "uint32_value": 30
                        }
                    }
                },
                "utterance": "",
                "analytics": {
                    "product_scenario": "IoTVoiceDiscovery",
                    "origin": "Scenario",
                    "purpose": "notify_about_discovery_start"
                }
            })")));
        const auto frame = TTypedSemanticFrameRequest{eventPayload};
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeIoTDiscoveryStartSemanticFrame("ssid", "password", "device_type", 30));
    }


    Y_UNIT_TEST(TypedSemanticFrameIoTDiscoverySuccessRequest) {
        const auto eventPayload = JsonToProto<google::protobuf::Struct>(JsonFromString(TStringBuf(R"({
                "typed_semantic_frame": {
                    "iot_discovery_success": {
                        "device_ids": {
                            "string_value": "device_id"
                        },
                        "product_ids": {
                            "string_value": "product_id"
                        },
                        "device_type": {
                            "string_value": "device_type"
                        }
                    }
                },
                "utterance": "",
                "analytics": {
                    "product_scenario": "IoTVoiceDiscovery",
                    "origin": "Scenario",
                    "purpose": "notify_about_discovery_success"
                }
            })")));
        const auto frame = TTypedSemanticFrameRequest{eventPayload};
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeIoTDiscoverySuccessSemanticFrame("device_id", "product_id", "device_type"));
    }

    Y_UNIT_TEST(TypedSemanticFrameIoTDiscoveryFailureRequest) {
        const auto eventPayload = JsonToProto<google::protobuf::Struct>(JsonFromString(TStringBuf(R"({
                "typed_semantic_frame": {
                    "iot_discovery_failure": {
                        "timeout_ms": {
                            "uint32_value": 30
                        },
                        "reason": {
                            "string_value": "some_reason"
                        },
                        "device_type": {
                            "string_value": "device_type"
                        }
                    }
                },
                "utterance": "",
                "analytics": {
                    "product_scenario": "IoTVoiceDiscovery",
                    "origin": "Scenario",
                    "purpose": "notify_about_discovery_failure"
                }
            })")));
        const auto frame = TTypedSemanticFrameRequest{eventPayload};
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeIoTDiscoveryFailureSemanticFrame(30, "some_reason", "device_type"));
    }

    Y_UNIT_TEST(TypedSemanticFrameMordoviaHomeScreenRequest) {
        const auto event = JsonToProto<TEvent>(JsonFromString(TStringBuf(R"({
            "payload": {
                "typed_semantic_frame": {
                    "mordovia_home_screen": {
                        "device_id": {
                            "string_value": "abcdefghijk"
                        }
                    }
                },
                "utterance": "главная для abcdefghijk",
                "analytics": {
                    "product_scenario": "Mordovia",
                    "origin": "SmartSpeaker",
                    "purpose": "get_main_screen"
                }
            },
            "name": "@@mm_semantic_frame",
            "type": "server_action"
        })")));
        const auto frame = TTypedSemanticFrameRequest{event.GetPayload()};
        UNIT_ASSERT_STRINGS_EQUAL(frame.Utterance, TStringBuf("главная для abcdefghijk"));
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeMordoviaHomeScreenSemanticFrame("abcdefghijk"));
    }

    Y_UNIT_TEST(TypedSemanticFrameGetCallerNameSemanticFrame) {
        const auto event = JsonToProto<TEvent>(JsonFromString(TStringBuf(R"({
            "payload": {
                "typed_semantic_frame": {
                    "get_caller_name": {
                    }
                },
                "utterance": "",
                "analytics": {
                    "product_scenario": "MessengerCall",
                    "origin": "SmartSpeaker",
                    "purpose": "say_caller_name"
                }
            },
            "name": "@@mm_semantic_frame",
            "type": "server_action"
        })")));
        const auto frame = TTypedSemanticFrameRequest{event.GetPayload()};
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeGetCallerNameSemanticFrame());
    }

    Y_UNIT_TEST(TypedSemanticFrameAddAccountSemanticFrame ) {
        const auto event = JsonToProto<TEvent>(JsonFromString(TStringBuf(R"({
            "payload": {
                "typed_semantic_frame": {
                    "multiaccount_add_account_semantic_frame": {
                        "token_type": {
                            "enum_value": "XToken"
                        },
                        "encrypted_code": {
                            "string_value": "c3Rya2d0b250cw=="
                        },
                        "signature": {
                            "string_value": "d252ZWx6YmhpcmlyZ2lobmF3YXh6cHVuY3h0Y291aXQ"
                        },
                        "encrypted_session_key": {
                            "string_value": "a291Y2dybXllaGR1aWdyZXJ5cHV0emNrcnB3dXh5d2c="
                        }
                    }
                },
                "utterance": "",
                "analytics": {
                    "origin": "Push",
                    "purpose": "multiaccount_add_account"
                }
            }
        })")));
        const auto frame = TTypedSemanticFrameRequest{event.GetPayload()};
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeAddAccountSemanticFrame("c3Rya2d0b250cw==", "d252ZWx6YmhpcmlyZ2lobmF3YXh6cHVuY3h0Y291aXQ", "a291Y2dybXllaGR1aWdyZXJ5cHV0emNrcnB3dXh5d2c="));
    }

    Y_UNIT_TEST(TypedSemanticFrameRemoveAccountSemanticFrame ) {
        const auto event = JsonToProto<TEvent>(JsonFromString(TStringBuf(R"({
            "payload": {
                "typed_semantic_frame": {
                    "multiaccount_remove_account_semantic_frame": {
                        "puid": {
                            "string_value": "123"
                        }
                    }
                },
                "utterance": "",
                "analytics": {
                    "origin": "Push",
                    "purpose": "multiaccount_remove_account"
                }
            }
        })")));
        const auto frame = TTypedSemanticFrameRequest{event.GetPayload()};
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeRemoveAccountSemanticFrame("123"));
    }

    Y_UNIT_TEST(TypedSemanticFrameCentaurCollectCardsSemanticFrame) {
        const auto event = JsonToProto<TEvent>(JsonFromString(TStringBuf(R"({
            "payload": {
                "typed_semantic_frame": {
                    "centaur_collect_cards": {
                        "carousel_id": {
                            "string_value": "abcdefghijk"
                        }
                    }
                },
                "utterance": "",
                "analytics": {
                    "product_scenario": "Centaur",
                    "origin": "SmartSpeaker",
                    "purpose": "centaur_collect_cards"
                }
            },
            "name": "@@mm_semantic_frame",
            "type": "server_action"
        })")));
        const auto frame = TTypedSemanticFrameRequest{event.GetPayload()};
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeCentaurCollectCardsSemanticFrame("abcdefghijk"));
    }

    Y_UNIT_TEST(TypedSemanticFrameCentaurCollectMainScreenSemanticFrame) {
        const auto event = JsonToProto<TEvent>(JsonFromString(TStringBuf(R"({
            "payload": {
                "typed_semantic_frame": {
                    "centaur_collect_main_screen": {
                    }
                },
                "utterance": "",
                "analytics": {
                    "product_scenario": "Centaur",
                    "origin": "SmartSpeaker",
                    "purpose": "alice.centaur.collect_main_screen"
                }
            },
            "name": "@@mm_semantic_frame",
            "type": "server_action"
        })")));
        const auto frame = TTypedSemanticFrameRequest{event.GetPayload()};
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeCentaurCollectMainScreenSemanticFrame());
    }

    Y_UNIT_TEST(TypedSemanticFrameCentaurCollectUpperShutterSemanticFrame) {
        const auto event = JsonToProto<TEvent>(JsonFromString(TStringBuf(R"({
            "payload": {
                "typed_semantic_frame": {
                    "centaur_collect_upper_shutter": {
                    }
                },
                "utterance": "",
                "analytics": {
                    "product_scenario": "Centaur",
                    "origin": "SmartSpeaker",
                    "purpose": "alice.centaur.collect_upper_shutter"
                }
            },
            "name": "@@mm_semantic_frame",
            "type": "server_action"
        })")));
        const auto frame = TTypedSemanticFrameRequest{event.GetPayload()};
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeCentaurCollectUpperShutterSemanticFrame());
    }

    Y_UNIT_TEST(TypedSemanticFrameCentaurGetCardSemanticFrame) {
        const auto event = JsonToProto<TEvent>(JsonFromString(TStringBuf(R"({
            "payload": {
                "typed_semantic_frame": {
                    "centaur_get_card": {
                        "carousel_id": {
                            "string_value": "abcdefghijk"
                        }
                    }
                },
                "utterance": "",
                "analytics": {
                    "product_scenario": "Centaur",
                    "origin": "SmartSpeaker",
                    "purpose": "centaur_get_card"
                }
            },
            "name": "@@mm_semantic_frame",
            "type": "server_action"
        })")));
        const auto frame = TTypedSemanticFrameRequest{event.GetPayload()};
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeCentaurGetCardSemanticFrame("abcdefghijk"));
    }

    Y_UNIT_TEST(TypedSemanticGetPhotoFrameSemanticFrame) {
        const auto event = JsonToProto<TEvent>(JsonFromString(TStringBuf(R"({
            "payload": {
                "typed_semantic_frame": {
                    "get_photo_frame": {
                        "carousel_id": {
                            "string_value": "abcdefghijk"
                        },
                        "photo_id": {
                            "num_value": 3
                        }
                    }
                },
                "utterance": "",
                "analytics": {
                    "product_scenario": "Dialogovo",
                    "origin": "SmartSpeaker",
                    "purpose": "get_photo_frame"
                }
            },
            "name": "@@mm_semantic_frame",
            "type": "server_action"
        })")));
        const auto frame = TTypedSemanticFrameRequest{event.GetPayload()};
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeGetPhotoFrameSemanticFrame("abcdefghijk", 3));
    }

    Y_UNIT_TEST(TypedSemanticFrameNewsRequest) {
        const auto event = JsonToProto<TEvent>(JsonFromString(TStringBuf(R"({
            "payload": {
                "typed_semantic_frame": {
                    "news_semantic_frame": {
                        "topic": {
                            "news_topic_value": "world"
                        },
                        "max_count": {
                            "num_value": 3
                        },
                        "skip_intro_and_ending": {
                            "bool_value": true
                        },
                        "provider": {
                            "news_provider_value": {
                                "news_source": "SRC",
                                "rubric": "RB"
                            }
                        },
                        "where": {
                            "where_value": "local"
                        },
                        "disable_voice_buttons": {
                            "bool_value": true
                        },
                        "go_back": {
                            "bool_value": true
                        }
                    }
                },
                "utterance": "Расскажи новости",
                "analytics": {
                    "product_scenario": "get_news",
                    "origin": "SmartSpeaker",
                    "purpose": "get news"
                }
            },
            "name": "@@mm_semantic_frame",
            "type": "server_action"
        })")));
        const auto frame = TTypedSemanticFrameRequest{event.GetPayload()};
        UNIT_ASSERT_STRINGS_EQUAL(frame.Utterance, TStringBuf("Расскажи новости"));
        NData::TNewsProvider provider;
        provider.SetNewsSource("SRC");
        provider.SetRubric("RB");
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeNewsSemanticFrame("world", 3, true, provider, "local", true, true));
    }

    Y_UNIT_TEST(TypedSemanticFrameMusicPlayRequest) {
        const auto event = JsonToProto<TEvent>(JsonFromString(TStringBuf(R"({
            "payload": {
                "typed_semantic_frame": {
                    "music_play_semantic_frame": {
                        "special_playlist": {
                            "special_playlist_value": "sport"
                        },
                        "disable_autoflow": {
                            "bool_value": true
                        },
                        "disable_nlg": {
                            "bool_value": true
                        },
                        "play_single_track": {
                            "bool_value": true
                        },
                        "track_offset_index": {
                            "num_value": 5
                        },
                        "playlist": {
                            "string_value": "playlist name"
                        },
                        "object_id": {
                            "string_value": "211604"
                        },
                        "object_type": {
                            "enum_value": "Album"
                        },
                        "start_from_track_id": {
                            "string_value": "38646012"
                        },
                        "alarm_id": {
                            "string_value": "1111ffff-11ff-11ff-11ff-111111ffffff"
                        },
                        "offset_sec": {
                            "double_value": 2.345
                        },
                        "disable_history": {
                            "bool_value": true
                        },
                        "room": {
                            "room_value": "everywhere"
                        },
                        "location": {
                            "user_iot_room_value": "kitchen"
                        }
                    }
                },
                "utterance": "Подкаст о спорте",
                "analytics": {
                    "product_scenario": "HollywoodMusic",
                    "origin": "SmartSpeaker",
                    "purpose": "music_play"
                }
            },
            "name": "@@mm_semantic_frame",
            "type": "server_action"
        })")));
        const auto frame = TTypedSemanticFrameRequest{event.GetPayload()};
        UNIT_ASSERT_STRINGS_EQUAL(frame.Utterance, TStringBuf("Подкаст о спорте"));
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeMusicPlaySemanticFrame("sport", true, true, true, 5, "playlist name"));
    }

    Y_UNIT_TEST(TestExternalSkillFixedActivateSemanticFrame) {
        const auto event = JsonToProto<TEvent>(JsonFromString(TStringBuf(R"({
            "payload": {
                "typed_semantic_frame": {
                    "external_skill_fixed_activate_semantic_frame": {
                        "activation_command": {
                            "string_value": "включи книгу маленький принц"
                        },
                        "fixed_skill_id": {
                            "string_value": "689f64c4-3134-42ba-8685-2b7cd8f06f4d"
                        },
                        "payload": {
                            "string_value": "\"book_id\": \"123\""
                        }
                    }
                },
                "utterance": "включи книгу маленький принц на литрес",
                "analytics": {
                    "product_scenario": "Dialogovo",
                    "origin": "SmartSpeaker",
                    "purpose": "external_skill_fixed_activate"
                }
            },
            "name": "@@mm_semantic_frame",
            "type": "server_action"
        })")));
        const auto frame = TTypedSemanticFrameRequest{event.GetPayload()};
        UNIT_ASSERT_STRINGS_EQUAL(frame.Utterance, TStringBuf("включи книгу маленький принц на литрес"));
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeExternalSkillFixedActivateSemanticFrame("включи книгу маленький принц", "689f64c4-3134-42ba-8685-2b7cd8f06f4d", "\"book_id\": \"123\""));
    }

    Y_UNIT_TEST(TypedSemanticFrameExternalSkillActivateRequest) {
        const auto event = JsonToProto<TEvent>(JsonFromString(TStringBuf(R"({
        "payload": {
            "typed_semantic_frame": {
                "external_skill_activate_semantic_frame": {
                    "activation_phrase": {
                        "string_value": "квест"
                    }
                }
            },
            "utterance": "квест",
            "analytics": {
                "product_scenario": "Dialogovo",
                "origin": "SmartSpeaker",
                "purpose": "external_skill_activate"
            }
        },
        "name": "@@mm_semantic_frame",
        "type": "server_action"
        })")));
        const auto frame = TTypedSemanticFrameRequest{event.GetPayload()};
        UNIT_ASSERT_STRINGS_EQUAL(frame.Utterance, TStringBuf("квест"));
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeExternalSkillActivateSemanticFrame("квест"));
    }

    Y_UNIT_TEST(TypedSemanticFrameVideoSelectionRequest) {
        const auto event = JsonToProto<TEvent>(JsonFromString(TStringBuf(R"({
            "payload": {
                "typed_semantic_frame": {
                    "select_video": {
                        "action": {
                            "video_selection_action_value": "play"
                        },
                        "video_index": {
                            "num_value": "3"
                        },
                        "silent_response": {
                            "bool_value": true
                        }
                    }
                },
                "analytics": {
                    "product_scenario": "video_commands",
                    "origin": "RemoteControl",
                    "purpose": "select_video_from_gallery"
                }
            },
            "name": "@@mm_semantic_frame",
            "type": "server_action"
        })")));
        const auto frame = TTypedSemanticFrameRequest{event.GetPayload()};
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeVideoSelectionSemanticFrame("play", 3, /* isSilentResponse */ true));
    }

    Y_UNIT_TEST(TypedSemanticFrameOpenCurrentVideoRequest) {
        const auto event = JsonToProto<TEvent>(JsonFromString(TStringBuf(R"({
            "payload": {
                "typed_semantic_frame": {
                    "open_current_video": {
                        "action": {
                            "video_selection_action_value": "play"
                        },
                        "silent_response": {
                            "bool_value": true
                        }
                    }
                },
                "analytics": {
                    "product_scenario": "video_commands",
                    "origin": "RemoteControl",
                    "purpose": "open_current_video"
                }
            },
            "name": "@@mm_semantic_frame",
            "type": "server_action"
        })")));
        const auto frame = TTypedSemanticFrameRequest{event.GetPayload()};
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeOpenCurrentVideoSemanticFrame("play", /* isSilentResponse */ true));
    }

    Y_UNIT_TEST(TypedSemanticFrameOpenCurrentTrailerRequest) {
        const auto event = JsonToProto<TEvent>(JsonFromString(TStringBuf(R"({
            "payload": {
                "typed_semantic_frame": {
                    "open_current_trailer": {
                        "silent_response": {
                            "bool_value": true
                        }
                    }
                },
                "analytics": {
                    "product_scenario": "video_commands",
                    "origin": "RemoteControl",
                    "purpose": "open_current_trailer"
                }
            },
            "name": "@@mm_semantic_frame",
            "type": "server_action"
        })")));
        const auto frame = TTypedSemanticFrameRequest{event.GetPayload()};
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeOpenCurrentTrailerSemanticFrame(/* isSilentResponse */ true));
    }

    Y_UNIT_TEST(TypedSemanticFrameVideoPlayerFinishedRequest) {
        const auto event = JsonToProto<TEvent>(JsonFromString(TStringBuf(R"({
            "payload": {
                "typed_semantic_frame": {
                    "quasar.video_player.finished": {
                        "silent_response": {
                            "bool_value": true
                        }
                    }
                },
                "analytics": {
                    "product_scenario": "player_next_track",
                    "origin": "SmartSpeaker",
                    "purpose": "video_player_finished"
                }
            },
            "name": "@@mm_semantic_frame",
            "type": "server_action"
        })")));
        const auto frame = TTypedSemanticFrameRequest{event.GetPayload()};
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeVideoPlayerFinishedSemanticFrame(/* isSilentResponse */ true));
    }

    Y_UNIT_TEST(TypedSemanticFrameVideoPaymentConfirmedRequest) {
        const auto event = JsonToProto<TEvent>(JsonFromString(TStringBuf(R"({
            "payload": {
                "typed_semantic_frame": {
                    "video_payment_confirmed": {
                        "silent_response": {
                            "bool_value": true
                        }
                    }
                },
                "analytics": {
                    "product_scenario": "video_commands",
                    "origin": "RemoteControl",
                    "purpose": "video_payment_confirmed"
                }
            },
            "name": "@@mm_semantic_frame",
            "type": "server_action"
        })")));
        const auto frame = TTypedSemanticFrameRequest{event.GetPayload()};
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeVideoPaymentConfirmedSemanticFrame(/* isSilentResponse */true));
    }

    Y_UNIT_TEST(TypedSemanticFrameOnboardingStartingCriticalUpdate) {
        const auto event = JsonToProto<TEvent>(JsonFromString(TStringBuf(R"({
            "payload": {
                "typed_semantic_frame": {
                    "onboarding_starting_critical_update": {
                        "is_first_setup": {
                            "bool_value": true
                        }
                    }
                },
                "analytics": {
                    "origin": "SmartSpeaker",
                    "purpose": "get_starting_critical_update_onboarding"
                }
            },
            "name": "@@mm_semantic_frame",
            "type": "server_action"
        })")));
        const auto frame = TTypedSemanticFrameRequest{event.GetPayload()};
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeOnboardingStartingCriticalUpdateSemanticFrame(/* isFirstSetup */ true));
    }

    Y_UNIT_TEST(TypedSemanticFrameOnboardingStartingConfigureSuccess) {
        const auto event = JsonToProto<TEvent>(JsonFromString(TStringBuf(R"({
            "payload": {
                "typed_semantic_frame": {
                    "onboarding_starting_configure_success": {}
                },
                "analytics": {
                    "origin": "SmartSpeaker",
                    "purpose": "get_starting_configure_success_onboarding"
                }
            },
            "name": "@@mm_semantic_frame",
            "type": "server_action"
        })")));
        const auto frame = TTypedSemanticFrameRequest{event.GetPayload()};
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeOnboardingStartingConfigureSuccessSemanticFrame());
    }

    Y_UNIT_TEST(TypedSemanticFrameRequest_ThrowErrorOnInvalidEvent) {
        const TVector<TStringBuf> rawEvents = {
            TStringBuf(R"({
                "payload": {
                    "typed_semantic_frame": {
                        "external_skill_activate_semantic_frame": {
                            "activation_phrase": {
                                "string_value": "квест"
                            }
                        }
                    },
                    "utterance": "квест"
                },
                "name": "@@mm_semantic_frame",
                "type": "server_action"
            })"),
            TStringBuf(R"({
                "payload": {
                    "typed_semantic_frame": {
                        "external_skill_activate_semantic_frame": {
                            "activation_phrase": {
                                "string_value": "квест"
                            }
                        }
                    },
                    "utterance": "квест",
                    "analytics": {
                        "product_scenario": "Dialogovo"
                    }
                },
                "name": "@@mm_semantic_frame",
                "type": "server_action"
            })"),
            TStringBuf(R"({
                "payload": {
                    "typed_semantic_frame": {
                        "external_skill_activate_semantic_frame": {
                            "activation_phrase": {
                                "string_value": "квест"
                            }
                        }
                    },
                    "utterance": "квест",
                    "analytics": {
                        "purpose": "alala"
                    }
                },
                "name": "@@mm_semantic_frame",
                "type": "server_action"
            })"),
            TStringBuf(R"({
                "payload": {
                    "typed_semantic_frame": {
                        "search_semantic_frame": {
                            "query": {
                                "string_value": "что такое ананас"
                            }
                        },
                    },
                    "utterance": "что такое ананас"
                },
                "name": "@@mm_semantic_frame",
                "type": "server_action"
            })"),
            TStringBuf(R"({
                "payload": {
                    "typed_semantic_frame": {
                        "invalid_frame": {
                            "query": {
                                "string_value": "что такое ананас"
                            }
                        },
                    },
                    "utterance": "что такое ананас"
                },
                "name": "@@mm_semantic_frame",
                "type": "server_action"
            })"),
            TStringBuf(R"({
                "payload": {
                    "typed_semantic_frame": {
                    },
                    "utterance": "что такое ананас"
                },
                "name": "@@mm_semantic_frame",
                "type": "server_action"
            })"),
            TStringBuf(R"({
                "name": "@@mm_semantic_frame",
                "type": "server_action"
            })"),
        };
        for (const auto& rawEvent : rawEvents) {
            const auto event = JsonToProto<TEvent>(JsonFromString(rawEvent));
            UNIT_ASSERT_EXCEPTION(TTypedSemanticFrameRequest{event.GetPayload()}, yexception);
        }
    }

    Y_UNIT_TEST(TestPlayerNextTrackSemanticFrame) {
        const auto eventPayload = JsonToProto<google::protobuf::Struct>(JsonFromString(TStringBuf(R"({
            "typed_semantic_frame": {
                "player_next_track_semantic_frame": {
                }
            },
            "analytics": {
                "product_scenario": "Video",
                "origin": "Scenario",
                "purpose": "video_player_next_track"
            }
        })")));

        const auto semanticFrame = TTypedSemanticFrameRequest{eventPayload};

        TSemanticFrame expectedSemanticFrame{};
        expectedSemanticFrame.SetName("personal_assistant.scenarios.player.next_track");
        expectedSemanticFrame.MutableTypedSemanticFrame()->
            MutablePlayerNextTrackSemanticFrame()->CopyFrom(TPlayerNextTrackSemanticFrame{});
        UNIT_ASSERT_MESSAGES_EQUAL(semanticFrame.SemanticFrame, expectedSemanticFrame);
    }

    Y_UNIT_TEST(TestPlayerPrevTrackSemanticFrame) {
        const auto eventPayload = JsonToProto<google::protobuf::Struct>(JsonFromString(TStringBuf(R"({
            "typed_semantic_frame": {
                "player_prev_track_semantic_frame": {
                }
            },
            "analytics": {
                "product_scenario": "Radio",
                "origin": "Scenario",
                "purpose": "radio_player_next_track"
            }
        })")));

        const auto semanticFrame = TTypedSemanticFrameRequest{eventPayload};

        TSemanticFrame expectedSemanticFrame{};
        expectedSemanticFrame.SetName("personal_assistant.scenarios.player.previous_track");
        expectedSemanticFrame.MutableTypedSemanticFrame()->
            MutablePlayerPrevTrackSemanticFrame()->CopyFrom(TPlayerPrevTrackSemanticFrame{});
        UNIT_ASSERT_MESSAGES_EQUAL(semanticFrame.SemanticFrame, expectedSemanticFrame);
    }

    Y_UNIT_TEST(TestPlayerLikeSemanticFrame) {
        const auto eventPayload = JsonToProto<google::protobuf::Struct>(JsonFromString(TStringBuf(R"({
            "typed_semantic_frame": {
                "player_like_semantic_frame": {
                    "content_id": {
                        "content_id_value": {
                            "type": "Album",
                            "id": "3475523"
                        }
                    }
                }
            },
            "utterance": "",
            "analytics": {
                "product_scenario": "Music",
                "origin": "Scenario",
                "purpose": "music_player_like"
            }
        })")));

        const auto semanticFrame = TTypedSemanticFrameRequest{eventPayload};

        TSemanticFrame expectedSemanticFrame{};
        expectedSemanticFrame.SetName("personal_assistant.scenarios.player.like");
        auto& slot = *expectedSemanticFrame.AddSlots();
        slot.SetName("content_id");
        slot.SetType("content_id");
        slot.SetValue(R"({"type":"Album","id":"3475523"})");
        slot.AddAcceptedTypes("content_id");

        auto& typedFrame = *expectedSemanticFrame.MutableTypedSemanticFrame()->MutablePlayerLikeSemanticFrame();
        auto& contentId = *typedFrame.MutableContentId()->MutableContentIdValue();
        contentId.SetType(NData::NMusic::TContentId_EContentType_Album);
        contentId.SetId("3475523"); // albumId of "Never Gonna Give You Up"

        UNIT_ASSERT_MESSAGES_EQUAL(semanticFrame.SemanticFrame, expectedSemanticFrame);
    }

    Y_UNIT_TEST(TestPlayerDislikeSemanticFrame) {
        const auto eventPayload = JsonToProto<google::protobuf::Struct>(JsonFromString(TStringBuf(R"({
            "typed_semantic_frame": {
                "player_dislike_semantic_frame": {
                    "content_id": {
                        "content_id_value": {
                            "type": "Album",
                            "id": "3475523"
                        }
                    }
                }
            },
            "analytics": {
                "product_scenario": "Video",
                "origin": "Scenario",
                "purpose": "video_player_like"
            }
        })")));

        const auto semanticFrame = TTypedSemanticFrameRequest{eventPayload};

        TSemanticFrame expectedSemanticFrame{};
        expectedSemanticFrame.SetName("personal_assistant.scenarios.player.dislike");
        auto& slot = *expectedSemanticFrame.AddSlots();
        slot.SetName("content_id");
        slot.SetType("content_id");
        slot.SetValue(R"({"type":"Album","id":"3475523"})");
        slot.AddAcceptedTypes("content_id");

        auto& typedFrame = *expectedSemanticFrame.MutableTypedSemanticFrame()->MutablePlayerDislikeSemanticFrame();
        auto& contentId = *typedFrame.MutableContentId()->MutableContentIdValue();
        contentId.SetType(NData::NMusic::TContentId_EContentType_Album);
        contentId.SetId("3475523"); // albumId of "Never Gonna Give You Up"

        UNIT_ASSERT_MESSAGES_EQUAL(semanticFrame.SemanticFrame, expectedSemanticFrame);
    }

    Y_UNIT_TEST(TestPlayerContinueSemanticFrame) {
        const auto eventPayload = JsonToProto<google::protobuf::Struct>(JsonFromString(TStringBuf(R"({
            "typed_semantic_frame": {
                "player_continue_semantic_frame": {
                    "disable_nlg": {
                        "bool_value": true
                    }
                }
            },
            "analytics": {
                "product_scenario": "music",
                "origin": "Scenario",
                "purpose": "music_continue"
            }
        })")));

        const auto semanticFrame = TTypedSemanticFrameRequest{eventPayload};

        TSemanticFrame expectedSemanticFrame{};
        expectedSemanticFrame.SetName("personal_assistant.scenarios.player.continue");

        auto& disableNlgSlot = *expectedSemanticFrame.AddSlots();
        disableNlgSlot.SetName("disable_nlg");
        disableNlgSlot.SetType("bool");
        disableNlgSlot.SetValue(ToString(true));
        disableNlgSlot.AddAcceptedTypes("bool");

        auto& tsf = *expectedSemanticFrame.MutableTypedSemanticFrame()->MutablePlayerContinueSemanticFrame();
        tsf.MutableDisableNlg()->SetBoolValue(true);

        UNIT_ASSERT_MESSAGES_EQUAL(semanticFrame.SemanticFrame, expectedSemanticFrame);
    }

    Y_UNIT_TEST(TestPlayerWhatIsPlayingSemanticFrame) {
        const auto eventPayload = JsonToProto<google::protobuf::Struct>(JsonFromString(TStringBuf(R"({
            "typed_semantic_frame": {
                "player_what_is_playing_semantic_frame": {
                }
            },
            "analytics": {
                "product_scenario": "music",
                "origin": "Scenario",
                "purpose": "music_what_is_playing"
            }
        })")));

        const auto semanticFrame = TTypedSemanticFrameRequest{eventPayload};

        TSemanticFrame expectedSemanticFrame{};
        expectedSemanticFrame.SetName("personal_assistant.scenarios.player.what_is_playing");
        expectedSemanticFrame.MutableTypedSemanticFrame()->
            MutablePlayerWhatIsPlayingSemanticFrame()->CopyFrom(TPlayerWhatIsPlayingSemanticFrame{});
        UNIT_ASSERT_MESSAGES_EQUAL(semanticFrame.SemanticFrame, expectedSemanticFrame);
    }

    Y_UNIT_TEST(TestPlayerShuffleSemanticFrame) {
        const auto eventPayload = JsonToProto<google::protobuf::Struct>(JsonFromString(TStringBuf(R"({
            "typed_semantic_frame": {
                "player_shuffle_semantic_frame": {
                }
            },
            "analytics": {
                "product_scenario": "music",
                "origin": "Scenario",
                "purpose": "music_shuffle"
            }
        })")));

        const auto semanticFrame = TTypedSemanticFrameRequest{eventPayload};

        TSemanticFrame expectedSemanticFrame{};
        expectedSemanticFrame.SetName("personal_assistant.scenarios.player.shuffle");
        expectedSemanticFrame.MutableTypedSemanticFrame()->
            MutablePlayerShuffleSemanticFrame()->CopyFrom(TPlayerShuffleSemanticFrame{});
        UNIT_ASSERT_MESSAGES_EQUAL(semanticFrame.SemanticFrame, expectedSemanticFrame);
    }

    Y_UNIT_TEST(TestPlayerReplaySemanticFrame) {
        const auto eventPayload = JsonToProto<google::protobuf::Struct>(JsonFromString(TStringBuf(R"({
            "typed_semantic_frame": {
                "player_replay_semantic_frame": {
                }
            },
            "analytics": {
                "product_scenario": "music",
                "origin": "Scenario",
                "purpose": "music_replay"
            }
        })")));

        const auto semanticFrame = TTypedSemanticFrameRequest{eventPayload};

        TSemanticFrame expectedSemanticFrame{};
        expectedSemanticFrame.SetName("personal_assistant.scenarios.player.replay");
        expectedSemanticFrame.MutableTypedSemanticFrame()->
            MutablePlayerReplaySemanticFrame()->CopyFrom(TPlayerReplaySemanticFrame{});
        UNIT_ASSERT_MESSAGES_EQUAL(semanticFrame.SemanticFrame, expectedSemanticFrame);
    }

    Y_UNIT_TEST(TestPlayerRewindSemanticFrame) {
        const auto eventPayload = JsonToProto<google::protobuf::Struct>(JsonFromString(TStringBuf(R"({
            "typed_semantic_frame": {
                "player_rewind_semantic_frame": {
                    "time": {
                        "units_time_value": "{\"hours\":5,\"minutes\":10}"
                    },
                    "rewind_type": {
                        "rewind_type_value": "forward"
                    },
                }
            },
            "analytics": {
                "product_scenario": "music",
                "origin": "Scenario",
                "purpose": "music_rewind"
            }
        })")));

        const auto semanticFrame = TTypedSemanticFrameRequest{eventPayload};

        TSemanticFrame expectedSemanticFrame{};
        expectedSemanticFrame.SetName("personal_assistant.scenarios.player.rewind");
        auto& frame = *expectedSemanticFrame.MutableTypedSemanticFrame()->
            MutablePlayerRewindSemanticFrame();
        frame.MutableTime()->SetUnitsTimeValue("{\"hours\":5,\"minutes\":10}");
        frame.MutableRewindType()->SetRewindTypeValue("forward");

        auto& timeSlot = *expectedSemanticFrame.AddSlots();
        timeSlot.SetName("time");
        timeSlot.SetType("sys.units_time");
        timeSlot.SetValue("{\"hours\":5,\"minutes\":10}");
        timeSlot.AddAcceptedTypes("sys.units_time");

        auto& rewindTypeSlot = *expectedSemanticFrame.AddSlots();
        rewindTypeSlot.SetName("rewind_type");
        rewindTypeSlot.SetType("custom.rewind_type");
        rewindTypeSlot.SetValue("forward");
        rewindTypeSlot.AddAcceptedTypes("custom.rewind_type");

        UNIT_ASSERT_MESSAGES_EQUAL(semanticFrame.SemanticFrame, expectedSemanticFrame);
    }

    Y_UNIT_TEST(TestPlayerRepeatSemanticFrame) {
        const auto eventPayload = JsonToProto<google::protobuf::Struct>(JsonFromString(TStringBuf(R"({
            "typed_semantic_frame": {
                "player_repeat_semantic_frame": {
                }
            },
            "analytics": {
                "product_scenario": "music",
                "origin": "Scenario",
                "purpose": "music_repeat"
            }
        })")));

        const auto semanticFrame = TTypedSemanticFrameRequest{eventPayload};

        TSemanticFrame expectedSemanticFrame{};
        expectedSemanticFrame.SetName("personal_assistant.scenarios.player.repeat");
        expectedSemanticFrame.MutableTypedSemanticFrame()->
            MutablePlayerRepeatSemanticFrame()->CopyFrom(TPlayerRepeatSemanticFrame{});
        UNIT_ASSERT_MESSAGES_EQUAL(semanticFrame.SemanticFrame, expectedSemanticFrame);
    }

    Y_UNIT_TEST(TestNotificationsSubscribeSemanticFrame) {
        const auto eventPayload = JsonToProto<google::protobuf::Struct>(JsonFromString(TStringBuf(R"({
            "typed_semantic_frame": {
                "notifications_subscribe_semantic_frame": {
                    "accept": { "string_value": "true" },
                    "notification_subscription": { "subscription_value": "{\"id\": \"1\"}" }
                }
            },
            "analytics": {
                "product_scenario": "NotificationsSubscribe",
                "origin": "Scenario",
                "purpose": "notifications_subscribe"
            }
        })")));

        const auto semanticFrame = TTypedSemanticFrameRequest{eventPayload};

        TSemanticFrame expectedSemanticFrame{};
        expectedSemanticFrame.SetName("alice.notifications_subscribe");

        auto& acceptSlot = *expectedSemanticFrame.AddSlots();
        acceptSlot.SetName("accept");
        acceptSlot.SetType("string");
        acceptSlot.SetValue("true");
        acceptSlot.AddAcceptedTypes("string");
        expectedSemanticFrame.MutableTypedSemanticFrame()->
            MutableNotificationsSubscribeSemanticFrame()->MutableAccept()->SetStringValue("true");

        auto& notificationSubscriptionSlot = *expectedSemanticFrame.AddSlots();
        notificationSubscriptionSlot.SetName("notification_subscription");
        notificationSubscriptionSlot.SetType("custom.notification_subscription");
        notificationSubscriptionSlot.SetValue("{\"id\": \"1\"}");
        notificationSubscriptionSlot.AddAcceptedTypes("custom.notification_subscription");
        expectedSemanticFrame.MutableTypedSemanticFrame()->
            MutableNotificationsSubscribeSemanticFrame()->MutableNotificationSubscription()->SetSubscriptionValue("{\"id\": \"1\"}");

        UNIT_ASSERT_MESSAGES_EQUAL(semanticFrame.SemanticFrame, expectedSemanticFrame);
    }

    Y_UNIT_TEST(TestVideoRaterSemanticFrame) {
        const auto eventPayload = JsonToProto<google::protobuf::Struct>(JsonFromString(TStringBuf(R"({
            "typed_semantic_frame": {
                "video_rater_semantic_frame": {
                }
            },
            "analytics": {
                "product_scenario": "VideoRater",
                "origin": "Scenario",
                "purpose": "rate_video"
            }
        })")));

        const auto semanticFrame = TTypedSemanticFrameRequest{eventPayload};

        TSemanticFrame expectedSemanticFrame{};
        expectedSemanticFrame.SetName("alice.video_rater.init");
        expectedSemanticFrame.MutableTypedSemanticFrame()->
            MutableVideoRaterSemanticFrame()->CopyFrom(TVideoRaterSemanticFrame{});
        UNIT_ASSERT_MESSAGES_EQUAL(semanticFrame.SemanticFrame, expectedSemanticFrame);
    }

    Y_UNIT_TEST(TestSetupRcuStatusSemanticFrame) {
        const auto event = JsonToProto<TEvent>(JsonFromString(TStringBuf(R"({
            "payload": {
                "typed_semantic_frame": {
                    "setup_rcu.status": {
                        "status": {
                            "enum_value": "Success"
                        }
                    }
                },
                "analytics": {
                    "product_scenario": "setup_rcu",
                    "origin": "RemoteControl",
                    "purpose": "setup_rcu_status"
                }
            },
            "name": "@@mm_semantic_frame",
            "type": "server_action"
        })")));


        const auto semanticFrame = TTypedSemanticFrameRequest{event.GetPayload()};

        UNIT_ASSERT_MESSAGES_EQUAL(semanticFrame.SemanticFrame, MakeSetupRcuStatusSemanticFrame(TSetupRcuStatusSlot::Success));
    }

    Y_UNIT_TEST(TestSetupRcuAutoStatusSemanticFrame) {
        const auto event = JsonToProto<TEvent>(JsonFromString(TStringBuf(R"({
            "payload": {
                "typed_semantic_frame": {
                    "setup_rcu_auto.status": {
                        "status": {
                            "enum_value": "Error"
                        }
                    }
                },
                "analytics": {
                    "product_scenario": "setup_rcu",
                    "origin": "RemoteControl",
                    "purpose": "setup_rcu_auto_status"
                }
            },
            "name": "@@mm_semantic_frame",
            "type": "server_action"
        })")));


        const auto semanticFrame = TTypedSemanticFrameRequest{event.GetPayload()};

        UNIT_ASSERT_MESSAGES_EQUAL(semanticFrame.SemanticFrame, MakeSetupRcuAutoStatusSemanticFrame(TSetupRcuStatusSlot::Error));
    }

    Y_UNIT_TEST(TestSetupRcuCheckStatusSemanticFrame) {
        const auto event = JsonToProto<TEvent>(JsonFromString(TStringBuf(R"({
            "payload": {
                "typed_semantic_frame": {
                    "setup_rcu_check.status": {
                        "status": {
                            "enum_value": "InactiveTimeout"
                        }
                    }
                },
                "analytics": {
                    "product_scenario": "setup_rcu",
                    "origin": "RemoteControl",
                    "purpose": "setup_rcu_check_status"
                }
            },
            "name": "@@mm_semantic_frame",
            "type": "server_action"
        })")));


        const auto semanticFrame = TTypedSemanticFrameRequest{event.GetPayload()};

        UNIT_ASSERT_MESSAGES_EQUAL(semanticFrame.SemanticFrame, MakeSetupRcuCheckStatusSemanticFrame(TSetupRcuStatusSlot::InactiveTimeout));
    }

    Y_UNIT_TEST(TestSetupRcuAdvancedStatusSemanticFrame) {
        const auto event = JsonToProto<TEvent>(JsonFromString(TStringBuf(R"({
            "payload": {
                "typed_semantic_frame": {
                    "setup_rcu_advanced.status": {
                        "status": {
                            "enum_value": "Success"
                        }
                    }
                },
                "analytics": {
                    "product_scenario": "setup_rcu",
                    "origin": "RemoteControl",
                    "purpose": "setup_rcu_advanced_status"
                }
            },
            "name": "@@mm_semantic_frame",
            "type": "server_action"
        })")));


        const auto semanticFrame = TTypedSemanticFrameRequest{event.GetPayload()};

        UNIT_ASSERT_MESSAGES_EQUAL(semanticFrame.SemanticFrame, MakeSetupRcuAdvancedStatusSemanticFrame(TSetupRcuStatusSlot::Success));
    }

    Y_UNIT_TEST(TestSetupRcuManualStartSemanticFrame) {
        const auto event = JsonToProto<TEvent>(JsonFromString(TStringBuf(R"({
            "payload": {
                "typed_semantic_frame": {
                    "setup_rcu_manual.start": {}
                },
                "analytics": {
                    "product_scenario": "setup_rcu",
                    "origin": "RemoteControl",
                    "purpose": "setup_rcu_manual_start"
                }
            },
            "name": "@@mm_semantic_frame",
            "type": "server_action"
        })")));

        const auto semanticFrame = TTypedSemanticFrameRequest{event.GetPayload()};

        UNIT_ASSERT_MESSAGES_EQUAL(semanticFrame.SemanticFrame, MakeSetupRcuManualStartSemanticFrame());
    }

    Y_UNIT_TEST(TestSetupRcuAutoStartSemanticFrame) {
        const auto event = JsonToProto<TEvent>(JsonFromString(TStringBuf(R"({
            "payload": {
                "typed_semantic_frame": {
                    "setup_rcu_auto.start": {
                        "tv_model": {
                            "string_value": "Samsung"
                        }
                    }
                },
                "analytics": {
                    "product_scenario": "setup_rcu",
                    "origin": "RemoteControl",
                    "purpose": "setup_rcu_auto_start"
                }
            },
            "name": "@@mm_semantic_frame",
            "type": "server_action"
        })")));

        const auto semanticFrame = TTypedSemanticFrameRequest{event.GetPayload()};

        UNIT_ASSERT_MESSAGES_EQUAL(semanticFrame.SemanticFrame, MakeSetupRcuAutoStartSemanticFrame("Samsung"));
    }

    Y_UNIT_TEST(TestLinkARemoteSemanticFrame) {
        const auto event = JsonToProto<TEvent>(JsonFromString(TStringBuf(R"({
            "payload": {
                "typed_semantic_frame": {
                    "link_a_remote": {
                        "link_type": {
                            "string_value": "ir"
                        }
                    }
                },
                "analytics": {
                    "product_scenario": "setup_rcu",
                    "origin": "RemoteControl",
                    "purpose": "setup_rcu_start"
                }
            },
            "name": "@@mm_semantic_frame",
            "type": "server_action"
        })")));

        const auto semanticFrame = TTypedSemanticFrameRequest{event.GetPayload()};

        UNIT_ASSERT_MESSAGES_EQUAL(semanticFrame.SemanticFrame, MakeLinkARemoteSemanticFrame("ir"));
    }

    Y_UNIT_TEST(TestRequestTechnicalSupportSemanticFrame) {
        const auto event = JsonToProto<TEvent>(JsonFromString(TStringBuf(R"({
            "payload": {
                "typed_semantic_frame": {
                    "request_technical_support": {}
                },
                "analytics": {
                    "product_scenario": "request_technical_support",
                    "origin": "RemoteControl",
                    "purpose": "request_technical_support"
                }
            },
            "name": "@@mm_semantic_frame",
            "type": "server_action"
        })")));

        const auto semanticFrame = TTypedSemanticFrameRequest{event.GetPayload()};

        UNIT_ASSERT_MESSAGES_EQUAL(semanticFrame.SemanticFrame, MakeRequestTechnicalSupportSemanticFrame());
    }

    Y_UNIT_TEST(TestHardcodedMorningShowSemanticFrame) {
        const auto event = JsonToProto<TEvent>(JsonFromString(TStringBuf(R"({
            "payload": {
                "typed_semantic_frame": {
                    "morning_show_semantic_frame": {
                        "offset": {
                            "num_value": "2"
                        },
                        "show_type": {
                            "string_value": "evening"
                        },
                        "news_provider": {
                            "data_value": "testProviderData"
                        },
                        "topic": {
                            "data_value": "testProviderTopic"
                        },
                        "next_track_index": {
                            "num_value": "5"
                        }
                    }
                },
                "analytics": {
                    "origin": "Scenario",
                    "purpose": "hardcoded_morning_show"
                }
            },
            "name": "@@mm_semantic_frame",
            "type": "server_action"
        })")));

        const auto semanticFrame = TTypedSemanticFrameRequest{event.GetPayload()};

        UNIT_ASSERT_MESSAGES_EQUAL(semanticFrame.SemanticFrame, MakeHardcodedMorningShowSemanticFrame(2, "evening", "testProviderData", "testProviderTopic", 5));
    }

    Y_UNIT_TEST(TestAlarmSetAliceShowSemanticFrame) {
        const auto event = JsonToProto<TEvent>(JsonFromString(TStringBuf(R"({
            "payload": {
                "typed_semantic_frame": {
                    "alarm_set_alice_show_semantic_frame": {}
                },
                "analytics": {
                    "origin": "Scenario",
                    "purpose": "alarm_set_alice_show",
                    "product_scenario": "alarm"
                }
            },
            "name": "@@mm_semantic_frame",
            "type": "server_action"
        })")));

        const auto frame = TTypedSemanticFrameRequest{event.GetPayload()};
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeAlarmSetAliceShowSemanticFrame());
    }

    Y_UNIT_TEST(TestTimeCapsuleNextStepSemanticFrame) {
        const auto event = JsonToProto<TEvent>(JsonFromString(TStringBuf(R"({
            "payload": {
                "typed_semantic_frame": {
                    "time_capsule_next_step_semantic_frame": {}
                },
                "analytics": {
                    "origin": "Scenario",
                    "purpose": "time_capsule.next_step",
                    "product_scenario": "time_capsule"
                }
            },
            "name": "@@mm_semantic_frame",
            "type": "server_action"
        })")));

        const auto frame = TTypedSemanticFrameRequest{event.GetPayload()};
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeTimeCapsuleNextStepSemanticFrame());
    }

    Y_UNIT_TEST(TestTimeCapsuleStopSemanticFrame) {
        const auto event = JsonToProto<TEvent>(JsonFromString(TStringBuf(R"({
            "payload": {
                "typed_semantic_frame": {
                    "time_capsule_stop_semantic_frame": {}
                },
                "analytics": {
                    "origin": "Scenario",
                    "purpose": "time_capsule.stop",
                    "product_scenario": "time_capsule"
                }
            },
            "name": "@@mm_semantic_frame",
            "type": "server_action"
        })")));

        const auto frame = TTypedSemanticFrameRequest{event.GetPayload()};
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeTimeCapsuleStopSemanticFrame());
    }

    Y_UNIT_TEST(TestTimeCapsuleStartSemanticFrame) {
        const auto event = JsonToProto<TEvent>(JsonFromString(TStringBuf(R"({
            "payload": {
                "typed_semantic_frame": {
                    "time_capsule_start_semantic_frame": {}
                },
                "analytics": {
                    "origin": "Scenario",
                    "purpose": "time_capsule.start",
                    "product_scenario": "time_capsule"
                }
            },
            "name": "@@mm_semantic_frame",
            "type": "server_action"
        })")));

        const auto frame = TTypedSemanticFrameRequest{event.GetPayload()};
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeTimeCapsuleStartSemanticFrame());
    }

    Y_UNIT_TEST(TestTimeCapsuleResumeSemanticFrame) {
        const auto event = JsonToProto<TEvent>(JsonFromString(TStringBuf(R"({
            "payload": {
                "typed_semantic_frame": {
                    "time_capsule_resume_semantic_frame": {}
                },
                "analytics": {
                    "origin": "Scenario",
                    "purpose": "time_capsule.resume",
                    "product_scenario": "time_capsule"
                }
            },
            "name": "@@mm_semantic_frame",
            "type": "server_action"
        })")));

        const auto frame = TTypedSemanticFrameRequest{event.GetPayload()};
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeTimeCapsuleResumeSemanticFrame());
    }

    Y_UNIT_TEST(TestTimeCapsuleSkipQuestionSemanticFrame) {
        const auto event = JsonToProto<TEvent>(JsonFromString(TStringBuf(R"({
            "payload": {
                "typed_semantic_frame": {
                    "time_capsule_skip_question_semantic_frame": {}
                },
                "analytics": {
                    "origin": "Scenario",
                    "purpose": "time_capsule.skip_question",
                    "product_scenario": "time_capsule"
                }
            },
            "name": "@@mm_semantic_frame",
            "type": "server_action"
        })")));

        const auto frame = TTypedSemanticFrameRequest{event.GetPayload()};
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeTimeCapsuleSkipQuestionSemanticFrame());
    }

    Y_UNIT_TEST(TypedSemanticFrameRadioPlayRequest) {
        const auto event = JsonToProto<TEvent>(JsonFromString(TStringBuf(R"({
            "payload": {
                "typed_semantic_frame": {
                    "radio_play_semantic_frame": {
                        "fm_radio": {
                            "fm_radio_value": "maximum"
                        },
                        "fm_radio_freq": {
                            "fm_radio_freq_value": "101.5"
                        },
                        "fm_radio_info": {
                            "fm_radio_info_value": {
                                "simple_nlu": true,
                                "fm_radio_ids": [
                                    "Радио Ваня",
                                    "Радио Шансон",
                                    "Energy FM"
                                ],
                                "content_metatag": "genre:disco",
                                "track_id": "01234567"
                            }
                        }
                    }
                },
                "utterance": "Радио максимум",
                "analytics": {
                    "product_scenario": "radio",
                    "origin": "SmartSpeaker",
                    "purpose": "radio_play"
                }
            },
            "name": "@@mm_semantic_frame",
            "type": "server_action"
        })")));
        const auto frame = TTypedSemanticFrameRequest{event.GetPayload()};
        UNIT_ASSERT_STRINGS_EQUAL(frame.Utterance, TStringBuf("Радио максимум"));
        NData::TFmRadioInfo fmRadioInfo;
        fmRadioInfo.SetSimpleNlu(true);
        fmRadioInfo.AddFmRadioIds("Радио Ваня");
        fmRadioInfo.AddFmRadioIds("Радио Шансон");
        fmRadioInfo.AddFmRadioIds("Energy FM");
        fmRadioInfo.SetContentMetatag("genre:disco");
        fmRadioInfo.SetTrackId("01234567");
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeRadioPlaySemanticFrame("maximum", "101.5", fmRadioInfo));
    }

    Y_UNIT_TEST(TypedSemanticFrameFmRadioPlayRequest) {
        const auto event = JsonToProto<TEvent>(JsonFromString(TStringBuf(R"({
            "payload": {
                "typed_semantic_frame": {
                    "fm_radio_play_semantic_frame": {
                        "fm_radio": {
                            "fm_radio_value": "maximum"
                        },
                        "fm_radio_freq": {
                            "fm_radio_freq_value": "101.5"
                        }
                    }
                },
                "utterance": "Радио максимум",
                "analytics": {
                    "product_scenario": "radio",
                    "origin": "SmartSpeaker",
                    "purpose": "radio_play"
                }
            },
            "name": "@@mm_semantic_frame",
            "type": "server_action"
        })")));
        const auto frame = TTypedSemanticFrameRequest{event.GetPayload()};
        UNIT_ASSERT_STRINGS_EQUAL(frame.Utterance, TStringBuf("Радио максимум"));
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeFmRadioPlaySemanticFrame("maximum", "101.5"));
    }

    Y_UNIT_TEST(TypedSemanticFrameSoundLouder) {
        const auto event = JsonToProto<TEvent>(JsonFromString(TStringBuf(R"({
            "payload": {
                "typed_semantic_frame": {
                    "sound_louder_semantic_frame": {}
                },
                "analytics": {
                    "origin": "Scenario",
                    "purpose": "sound_louder"
                }
            },
            "name": "@@mm_semantic_frame",
            "type": "server_action"
        })")));
        const auto frame = TTypedSemanticFrameRequest{event.GetPayload()};
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeSoundLouderSemanticFrame());
    }

    Y_UNIT_TEST(TypedSemanticFrameSoundQuiter) {
        const auto event = JsonToProto<TEvent>(JsonFromString(TStringBuf(R"({
            "payload": {
                "typed_semantic_frame": {
                    "sound_quiter_semantic_frame": {}
                },
                "analytics": {
                    "origin": "Scenario",
                    "purpose": "sound_quiter"
                }
            },
            "name": "@@mm_semantic_frame",
            "type": "server_action"
        })")));
        const auto frame = TTypedSemanticFrameRequest{event.GetPayload()};
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeSoundQuiterSemanticFrame());
    }

    Y_UNIT_TEST(TypedSemanticFrameSoundSetLevelNum) {
        const auto event = JsonToProto<TEvent>(JsonFromString(TStringBuf(R"({
            "payload": {
                "typed_semantic_frame": {
                    "sound_set_level_semantic_frame": {
                        "level": {
                            "num_level_value": "3"
                        }
                    }
                },
                "analytics": {
                    "origin": "Scenario",
                    "purpose": "sound_set_level"
                }
            },
            "name": "@@mm_semantic_frame",
            "type": "server_action"
        })")));
        const auto frame = TTypedSemanticFrameRequest{event.GetPayload()};
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeSoundSetLevelSemanticFrameNum(3));
    }

    Y_UNIT_TEST(TypedSemanticFrameSoundSetLevelFloat) {
        const auto event = JsonToProto<TEvent>(JsonFromString(TStringBuf(R"({
            "payload": {
                "typed_semantic_frame": {
                    "sound_set_level_semantic_frame": {
                        "level": {
                            "float_level_value": "5.5"
                        }
                    }
                },
                "analytics": {
                    "origin": "Scenario",
                    "purpose": "sound_set_level"
                }
            },
            "name": "@@mm_semantic_frame",
            "type": "server_action"
        })")));
        const auto frame = TTypedSemanticFrameRequest{event.GetPayload()};
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeSoundSetLevelSemanticFrameFloat(5.5));
    }

    Y_UNIT_TEST(TypedSemanticFrameGetTime) {
        const auto event = JsonToProto<TEvent>(JsonFromString(TStringBuf(R"({
            "payload": {
                "typed_semantic_frame": {
                    "get_time_semantic_frame": {
                        "where": {
                            "special_location_value": "nearest"
                        }
                    }
                },
                "analytics": {
                    "origin": "Scenario",
                    "purpose": "get_time"
                }
            },
            "name": "@@mm_semantic_frame",
            "type": "server_action"
        })")));
        const auto frame = TTypedSemanticFrameRequest{event.GetPayload()};
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeGetTimeSemanticFrame());
    }

    Y_UNIT_TEST(TypedSemanticFrameHowToSubscribe) {
        const auto event = JsonToProto<TEvent>(JsonFromString(TStringBuf(R"({
            "payload": {
                "typed_semantic_frame": {
                    "how_to_subscribe_semantic_frame": {}
                },
                "analytics": {
                    "origin": "Scenario",
                    "purpose": "how_to_subscribe"
                }
            },
            "name": "@@mm_semantic_frame",
            "type": "server_action"
        })")));
        const auto frame = TTypedSemanticFrameRequest{event.GetPayload()};
        TSemanticFrame expectedFrame;
        expectedFrame.SetName("alice.subscriptions.how_to_subscribe");
        expectedFrame.MutableTypedSemanticFrame()->MutableHowToSubscribeSemanticFrame();
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, expectedFrame);
    }

    Y_UNIT_TEST(TypedSemanticFrameMusicOnboarding) {
        const auto event = JsonToProto<TEvent>(JsonFromString(TStringBuf(R"({
            "payload": {
                "typed_semantic_frame": {
                    "music_onboarding_semantic_frame": {}
                },
                "analytics": {
                    "origin": "Scenario",
                    "purpose": "music_onboarding"
                }
            },
            "name": "@@mm_semantic_frame",
            "type": "server_action"
        })")));
        const auto frame = TTypedSemanticFrameRequest{event.GetPayload()};
        TSemanticFrame expectedFrame;
        expectedFrame.SetName("alice.music_onboarding");
        expectedFrame.MutableTypedSemanticFrame()->MutableMusicOnboardingSemanticFrame();
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, expectedFrame);
    }

    Y_UNIT_TEST(TypedSemanticFrameMusicOnboardingArtists) {
        const auto event = JsonToProto<TEvent>(JsonFromString(TStringBuf(R"({
            "payload": {
                "typed_semantic_frame": {
                    "music_onboarding_artists_semantic_frame": {}
                },
                "analytics": {
                    "origin": "Scenario",
                    "purpose": "music_onboarding_artists"
                }
            },
            "name": "@@mm_semantic_frame",
            "type": "server_action"
        })")));
        const auto frame = TTypedSemanticFrameRequest{event.GetPayload()};
        TSemanticFrame expectedFrame;
        expectedFrame.SetName("alice.music_onboarding.artists");
        expectedFrame.MutableTypedSemanticFrame()->MutableMusicOnboardingArtistsSemanticFrame();
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, expectedFrame);
    }

    Y_UNIT_TEST(TypedSemanticFrameMusicOnboardingGenres) {
        const auto event = JsonToProto<TEvent>(JsonFromString(TStringBuf(R"({
            "payload": {
                "typed_semantic_frame": {
                    "music_onboarding_genres_semantic_frame": {}
                },
                "analytics": {
                    "origin": "Scenario",
                    "purpose": "music_onboarding_genres"
                }
            },
            "name": "@@mm_semantic_frame",
            "type": "server_action"
        })")));
        const auto frame = TTypedSemanticFrameRequest{event.GetPayload()};
        TSemanticFrame expectedFrame;
        expectedFrame.SetName("alice.music_onboarding.genres");
        expectedFrame.MutableTypedSemanticFrame()->MutableMusicOnboardingGenresSemanticFrame();
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, expectedFrame);
    }

    Y_UNIT_TEST(TypedSemanticFrameMusicOnboardingTracks) {
        const auto event = JsonToProto<TEvent>(JsonFromString(TStringBuf(R"({
            "payload": {
                "typed_semantic_frame": {
                    "music_onboarding_tracks_semantic_frame": {}
                },
                "analytics": {
                    "origin": "Scenario",
                    "purpose": "music_onboarding_tracks"
                }
            },
            "name": "@@mm_semantic_frame",
            "type": "server_action"
        })")));
        const auto frame = TTypedSemanticFrameRequest{event.GetPayload()};
        TSemanticFrame expectedFrame;
        expectedFrame.SetName("alice.music_onboarding.tracks");
        expectedFrame.MutableTypedSemanticFrame()->MutableMusicOnboardingTracksSemanticFrame();
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, expectedFrame);
    }

    Y_UNIT_TEST(TypedSemanticFrameIoTScenariosPhraseAction) {
        const auto eventPayload = JsonToProto<google::protobuf::Struct>(JsonFromString(TStringBuf(R"({
                "typed_semantic_frame": {
                    "iot_scenarios_phrase_action_semantic_frame": {
                        "phrase": {
                            "string_value": "какая-то фраза"
                        }
                    }
                },
                "utterance": "",
                "analytics": {
                    "product_scenario": "IoTVoiceDiscovery",
                    "origin": "Scenario",
                    "purpose": "send_phrase_to_speak"
                }
            })")));
        const auto frame = TTypedSemanticFrameRequest{eventPayload};
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeIoTScenariosPhraseActionSemanticFrame("какая-то фраза"));
    }

    Y_UNIT_TEST(TypedSemanticFrameIoTScenariosTextAction) {
        const auto eventPayload = JsonToProto<google::protobuf::Struct>(JsonFromString(TStringBuf(R"({
                "typed_semantic_frame": {
                    "iot_scenarios_text_action_semantic_frame": {
                        "text": {
                            "string_value": "какая-то команда"
                        }
                    }
                },
                "utterance": "",
                "analytics": {
                    "product_scenario": "IoTVoiceDiscovery",
                    "origin": "Scenario",
                    "purpose": "send_text_command_to_execute"
                }
            })")));
        const auto frame = TTypedSemanticFrameRequest{eventPayload};
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeIoTScenariosTextActionSemanticFrame("какая-то команда"));
    }

    Y_UNIT_TEST(TypedSemanticFrameIoTScenariosLaunchAction) {
        const auto eventPayload = JsonToProto<google::protobuf::Struct>(JsonFromString(TStringBuf(R"({
                "typed_semantic_frame": {
                    "iot_scenarios_launch_action_semantic_frame": {
                        "launch_id": {
                            "string_value": "some-id"
                        },
                        "step_index": {
                            "uint32_value": 5
                        },
                        "instance": {
                            "string_value": "some-instance",
                        },
                        "value": {
                            "string_value": "some-value"
                        }
                    }
                },
                "analytics": {
                    "product_scenario": "IoTScenarios",
                    "origin": "Scenario",
                    "purpose": "send_launch_action_to_speaker"
                }
            })")));
        const auto frame = TTypedSemanticFrameRequest{eventPayload};
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, MakeIoTScenariosLaunchActionSemanticFrame("some-id", 5, "some-instance", "some-value"));
    }

    Y_UNIT_TEST(TypedSemanticFrameWhisperSaySomething) {
        const auto eventPayload = JsonToProto<google::protobuf::Struct>(JsonFromString(TStringBuf(R"({
            "typed_semantic_frame": {
                "whisper_say_something_semantic_frame": {}
            },
            "analytics": {
                "origin": "Scenario",
                "purpose": "whisper_say_something"
            }
        })")));
        const auto frame = TTypedSemanticFrameRequest{eventPayload};
        TSemanticFrame expectedFrame;
        expectedFrame.SetName("alice.voice.whisper.say_something");
        expectedFrame.MutableTypedSemanticFrame()->MutableWhisperSaySomethingSemanticFrame();
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, expectedFrame);
    }

    Y_UNIT_TEST(TypedSemanticFrameWhisperTurnOff) {
        const auto eventPayload = JsonToProto<google::protobuf::Struct>(JsonFromString(TStringBuf(R"({
            "typed_semantic_frame": {
                "whisper_turn_off_semantic_frame": {}
            },
            "analytics": {
                "origin": "Scenario",
                "purpose": "whisper_turn_off"
            }
        })")));
        const auto frame = TTypedSemanticFrameRequest{eventPayload};
        TSemanticFrame expectedFrame;
        expectedFrame.SetName("alice.voice.whisper.turn_off");
        expectedFrame.MutableTypedSemanticFrame()->MutableWhisperTurnOffSemanticFrame();
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, expectedFrame);
    }

    Y_UNIT_TEST(TypedSemanticFrameWhisperTurnOn) {
        const auto eventPayload = JsonToProto<google::protobuf::Struct>(JsonFromString(TStringBuf(R"({
            "typed_semantic_frame": {
                "whisper_turn_on_semantic_frame": {}
            },
            "analytics": {
                "origin": "Proactivity",
                "purpose": "whisper_turn_on"
            }
        })")));
        const auto frame = TTypedSemanticFrameRequest{eventPayload};
        TSemanticFrame expectedFrame;
        expectedFrame.SetName("alice.voice.whisper.turn_on");
        expectedFrame.MutableTypedSemanticFrame()->MutableWhisperTurnOnSemanticFrame();
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, expectedFrame);
    }

    Y_UNIT_TEST(TypedSemanticFrameWhisperWhatIsIt) {
        const auto eventPayload = JsonToProto<google::protobuf::Struct>(JsonFromString(TStringBuf(R"({
            "typed_semantic_frame": {
                "whisper_what_is_it_semantic_frame": {}
            },
            "analytics": {
                "origin": "Scenario",
                "purpose": "whisper_what_is_it"
            }
        })")));
        const auto frame = TTypedSemanticFrameRequest{eventPayload};
        TSemanticFrame expectedFrame;
        expectedFrame.SetName("alice.voice.whisper.what_is_it");
        expectedFrame.MutableTypedSemanticFrame()->MutableWhisperWhatIsItSemanticFrame();
        UNIT_ASSERT_MESSAGES_EQUAL(frame.SemanticFrame, expectedFrame);
    }

    Y_UNIT_TEST(FillAnalyticsWithLocation) {
        NiceMock<TMockContext> ctx{};
        NiceMock<TMockWalkerRequestContextTestWrapper> walkerRequestContextTestWrapper{};
        TCommonScenarioWalker scenarioWalker(walkerRequestContextTestWrapper.Get().GlobalCtx());
        NMegamind::TMegamindAnalyticsInfoBuilder analyticsInfoBuilder{};
        TScenarioWrapperPtrs wrappers{};
        const auto skr = TSpeechKitRequestBuilder(TStringBuf(R"({"request":{"event":{"type":"text_input"}, "location": {"lat": 1, "lon": 2, "accuracy": 3}}})")).Build();
        const auto request = CreateRequest(IEvent::CreateEvent(skr.Event()), skr);
        scenarioWalker.PostFillAnalyticsInfo(analyticsInfoBuilder, ctx, wrappers, /* wrapper= */ nullptr,
                                             /* scenariosWithTunnellerResponses= */ {},
                                             request);
        NMegamind::TMegamindAnalyticsInfo expectedAnalyticsInfo;
        const auto status = google::protobuf::util::JsonStringToMessage(
            R"({"location":{"speed":0,"lat":1,"lon":2,"recency":0,"accuracy":3}})", &expectedAnalyticsInfo);
        UNIT_ASSERT(status.ok());
        UNIT_ASSERT_MESSAGES_EQUAL(expectedAnalyticsInfo, analyticsInfoBuilder.BuildProto());
    }

    Y_UNIT_TEST(FillAnalyticsInfoWithoutSelectedWrapper) {
        const TString UTTERANCE = "utterance";
        const TMaybe<TStringBuf> SHOWN_UTTERANCE;
        const TString VINSLESS_SCENARIO_NAME = "alice.response";
        const TString VINSLESS_TUNNELLER_RESPONSE = "Response tunneller raws";

        StrictMock<TMockWalkerRequestContextTestWrapper> walkerRequestContextTestWrapper;
        TMockContext ctx;
        TMockResponses responses;
        TSearchResponse searchObject(TString{MEGAMIND_TUNNELLER_RESPONSE}, "", TRTLogger::NullLogger(),
                                     /* initDataSources= */ false);
        EXPECT_CALL(responses, WebSearchResponse(_)).WillRepeatedly(ReturnRef(searchObject));

        TScenarioConfig vinslessConfig;
        vinslessConfig.SetName(VINSLESS_SCENARIO_NAME);

        TConfigBasedAppHostPureProtocolScenario vinslessScenario(vinslessConfig);

        TBlackBoxFullUserInfoProto blackBoxResponse;
        blackBoxResponse.MutableUserInfo()->AddSubscriptions("kinopoisk");
        blackBoxResponse.MutableUserInfo()->SetHasYandexPlus(true);
        EXPECT_CALL(responses, BlackBoxResponse(_)).WillRepeatedly(ReturnRef(blackBoxResponse));

        EXPECT_CALL(ctx, HasResponses()).WillRepeatedly(Return(true));
        EXPECT_CALL(ctx, Responses()).WillRepeatedly(ReturnRef(responses));
        EXPECT_CALL(ctx, PolyglotUtterance()).WillRepeatedly(Return(UTTERANCE));
        EXPECT_CALL(ctx, AsrNormalizedUtterance()).WillRepeatedly(Return(SHOWN_UTTERANCE));
        auto skr = TSpeechKitRequestBuilder(TStringBuf("{}"))
                       .SetProtoPatcher([&](NMegamind::TSpeechKitInitContext& ctx) {
                           auto& actions = *ctx.Proto->MutableRequest()->MutableDeviceState()->MutableActions();
                           auto& action = actions["action_id"];
                           action.MutableNluHint()->MutableInstances()->Add()->SetPhrase("my_phrase");
                           action.MutableDirectives()->Add()->SetName("my_directive");
                       })
                       .Build();
        EXPECT_CALL(ctx, SpeechKitRequest()).WillRepeatedly(Return(skr));
        TClientInfo clientInfo{TClientInfoProto{}};
        EXPECT_CALL(ctx, ClientInfo()).WillRepeatedly(ReturnRef(clientInfo));
        TMockSession session{};

        EXPECT_CALL(ctx, Session()).WillRepeatedly(Return(&session));
        EXPECT_CALL(ctx, HasIoTUserInfo()).WillRepeatedly(Return(true));

        TIoTUserInfo ioTUserInfo;
        auto& color = *ioTUserInfo.AddColors();
        color.SetId("raspberry");
        color.SetName("Малиновый");
        EXPECT_CALL(ctx, IoTUserInfo()).WillRepeatedly(ReturnRef(ioTUserInfo));

        TScenarioWrapperPtrs wrappers;
        {
            TScenarioInfraConfig scenarioConfig;
            TMockContext applyContext;
            EXPECT_CALL(applyContext, ScenarioConfig(_)).WillRepeatedly(ReturnRef(scenarioConfig));
            EXPECT_CALL(applyContext, Logger()).WillRepeatedly(ReturnRef(TRTLogger::NullLogger()));
            EXPECT_CALL(applyContext, Sensors())
                .WillRepeatedly(ReturnRef(walkerRequestContextTestWrapper.Get().GlobalCtx().ServiceSensors()));
            EXPECT_CALL(applyContext, ClientInfo()).WillRepeatedly(ReturnRef(clientInfo));
            const IScenarioWrapper::TSemanticFrames& requestFrames = {};
            auto wrapper = MakeIntrusive<TAppHostPureProtocolScenarioWrapper>(
                vinslessScenario, applyContext, requestFrames, NMegamind::TGuidGenerator{},
                EDeferredApplyMode::DeferredCall, /* restoreAllFromSession= */ true, walkerRequestContextTestWrapper.Get().ItemProxyAdapter());

            wrapper->GetUserInfo().CreateScenarioUserInfoBuilder()->AddProfile(
                "response.user.info", "response_user_info", "some response description");
            wrapper->GetAnalyticsInfo()
                .CreateScenarioAnalyticsInfoBuilder()
                ->SetIntentName("alice.response.intent")
                .AddObject("response.object.id", "response_some_object", "yet another response description")
                .AddTunnellerRawResponse(VINSLESS_TUNNELLER_RESPONSE);

            wrappers.emplace_back(std::move(wrapper));
        }

        NMegamind::TMegamindAnalyticsInfo expectedAnalyticsInfo;
        const auto status = google::protobuf::util::JsonStringToMessage(
            TString{MEGAMIND_ANALYTICS_INFO_WITHOUT_WINNER_SCENARIO}, &expectedAnalyticsInfo);
        UNIT_ASSERT(status.ok());

        TCommonScenarioWalker scenarioWalker(walkerRequestContextTestWrapper.Get().GlobalCtx());
        NMegamind::TMegamindAnalyticsInfoBuilder analyticsInfoBuilder;
        scenarioWalker.PreFillAnalyticsInfo(analyticsInfoBuilder, ctx);
        scenarioWalker.PostFillAnalyticsInfo(analyticsInfoBuilder, ctx, wrappers, /* wrapper= */ nullptr,
                                             {VINSLESS_SCENARIO_NAME, NImpl::MEGAMIND_TUNNELLER_RESPONSE_KEY},
                                             walkerRequestContextTestWrapper.GetRequest());

        UNIT_ASSERT_MESSAGES_EQUAL(expectedAnalyticsInfo, analyticsInfoBuilder.BuildProto());
    }

    Y_UNIT_TEST(FillAnalyticsInfoWithSelectedWrapper) {
        const TString UTTERANCE = "utterance";
        const TMaybe<TStringBuf> SHOWN_UTTERANCE;
        const TString VINSLESS_SCENARIO_NAME = "alice.response";
        const TString VINSLESS_TUNNELLER_RESPONSE = "Response tunneller raws";

        StrictMock<TMockWalkerRequestContextTestWrapper> walkerRequestContextTestWrapper;
        TMockContext ctx;
        TMockResponses responses;
        TSearchResponse searchObject(TString{MEGAMIND_TUNNELLER_RESPONSE}, "", TRTLogger::NullLogger(),
                                     /* initDataSources= */ false);
        EXPECT_CALL(responses, WebSearchResponse(_)).WillRepeatedly(ReturnRef(searchObject));

        TScenarioConfig vinslessConfig;
        vinslessConfig.SetName(VINSLESS_SCENARIO_NAME);

        TConfigBasedAppHostPureProtocolScenario vinslessScenario(vinslessConfig);

        EXPECT_CALL(ctx, Responses()).WillRepeatedly(ReturnRef(responses));
        EXPECT_CALL(ctx, PolyglotUtterance()).WillRepeatedly(Return(UTTERANCE));
        EXPECT_CALL(ctx, AsrNormalizedUtterance()).WillRepeatedly(Return(SHOWN_UTTERANCE));
        auto skr = TSpeechKitRequestBuilder(TStringBuf("{}")).Build();
        EXPECT_CALL(ctx, SpeechKitRequest()).WillRepeatedly(Return(skr));
        TClientInfo clientInfo{TClientInfoProto{}};
        EXPECT_CALL(ctx, ClientInfo()).WillRepeatedly(ReturnRef(clientInfo));

        TScenarioWrapperPtrs wrappers;
        {
            TScenarioInfraConfig scenarioConfig;
            TMockContext applyContext;
            EXPECT_CALL(applyContext, ScenarioConfig(_)).WillRepeatedly(ReturnRef(scenarioConfig));
            EXPECT_CALL(applyContext, Logger()).WillRepeatedly(ReturnRef(TRTLogger::NullLogger()));
            EXPECT_CALL(applyContext, Sensors())
                .WillRepeatedly(ReturnRef(walkerRequestContextTestWrapper.Get().GlobalCtx().ServiceSensors()));
            EXPECT_CALL(applyContext, ClientInfo()).WillRepeatedly(ReturnRef(clientInfo));
            const IScenarioWrapper::TSemanticFrames& requestFrames = {};
            auto wrapper = MakeIntrusive<TAppHostPureProtocolScenarioWrapper>(
                vinslessScenario, applyContext, requestFrames, NMegamind::TGuidGenerator{},
                EDeferredApplyMode::DeferredCall, /* restoreAllFromSession= */ true,
                walkerRequestContextTestWrapper.Get().ItemProxyAdapter());

            wrapper->GetUserInfo().CreateScenarioUserInfoBuilder()->AddProfile(
                "response.user.info", "response_user_info", "some response description");
            wrapper->GetAnalyticsInfo()
                .CreateScenarioAnalyticsInfoBuilder()
                ->SetIntentName("alice.response.intent")
                .AddObject("response.object.id", "response_some_object", "yet another response description")
                .AddTunnellerRawResponse(VINSLESS_TUNNELLER_RESPONSE);

            wrappers.emplace_back(std::move(wrapper));
        }

        NMegamind::TMegamindAnalyticsInfo expectedAnalyticsInfo;
        const auto status = google::protobuf::util::JsonStringToMessage(
            TString{MEGAMIND_ANALYTICS_INFO_WITH_WINNER_SCENARIO}, &expectedAnalyticsInfo);
        UNIT_ASSERT(status.ok());

        TCommonScenarioWalker scenarioWalker(walkerRequestContextTestWrapper.Get().GlobalCtx());
        NMegamind::TMegamindAnalyticsInfoBuilder analyticsInfoBuilder;
        scenarioWalker.PreFillAnalyticsInfo(analyticsInfoBuilder, ctx);
        scenarioWalker.PostFillAnalyticsInfo(analyticsInfoBuilder, ctx, wrappers, wrappers.front(),
                                             {NImpl::ALL_SCENARIOS}, walkerRequestContextTestWrapper.GetRequest());

        UNIT_ASSERT_MESSAGES_EQUAL(expectedAnalyticsInfo, analyticsInfoBuilder.BuildProto());
    }

    Y_UNIT_TEST(FillAnalyticsInfoWithSelectedWrapperAndEmptyAnalyticsInfo) {
        const TString UTTERANCE = "utterance";
        const TMaybe<TStringBuf> SHOWN_UTTERANCE;
        const TString VINSLESS_SCENARIO_NAME = "alice.response";

        StrictMock<TMockWalkerRequestContextTestWrapper> walkerRequestContextTestWrapper;
        TMockContext ctx;
        TMockResponses responses;
        TSearchResponse searchObject(TString{MEGAMIND_TUNNELLER_RESPONSE}, "", TRTLogger::NullLogger(),
                                     /* initDataSources= */ false);
        EXPECT_CALL(responses, WebSearchResponse(_)).WillRepeatedly(ReturnRef(searchObject));

        TScenarioConfig vinslessConfig;
        vinslessConfig.SetName(VINSLESS_SCENARIO_NAME);

        TConfigBasedAppHostPureProtocolScenario vinslessScenario(vinslessConfig);

        EXPECT_CALL(ctx, Responses()).WillRepeatedly(ReturnRef(responses));
        EXPECT_CALL(ctx, PolyglotUtterance()).WillRepeatedly(Return(UTTERANCE));
        EXPECT_CALL(ctx, AsrNormalizedUtterance()).WillRepeatedly(Return(SHOWN_UTTERANCE));
        auto skr = TSpeechKitRequestBuilder(TStringBuf("{}")).Build();
        EXPECT_CALL(ctx, SpeechKitRequest()).WillRepeatedly(Return(skr));
        TClientInfo clientInfo{TClientInfoProto{}};
        EXPECT_CALL(ctx, ClientInfo()).WillRepeatedly(ReturnRef(clientInfo));

        TScenarioWrapperPtrs wrappers;
        {
            TScenarioInfraConfig scenarioConfig;
            TMockContext applyContext;
            EXPECT_CALL(applyContext, ScenarioConfig(_)).WillRepeatedly(ReturnRef(scenarioConfig));
            EXPECT_CALL(applyContext, Logger()).WillRepeatedly(ReturnRef(TRTLogger::NullLogger()));
            EXPECT_CALL(applyContext, Sensors())
                .WillRepeatedly(ReturnRef(walkerRequestContextTestWrapper.Get().GlobalCtx().ServiceSensors()));
            EXPECT_CALL(applyContext, ClientInfo()).WillRepeatedly(ReturnRef(clientInfo));
            const IScenarioWrapper::TSemanticFrames& requestFrames = {};
            auto wrapper = MakeIntrusive<TAppHostPureProtocolScenarioWrapper>(
                vinslessScenario, applyContext, requestFrames, NMegamind::TGuidGenerator{},
                EDeferredApplyMode::DeferredCall, /* restoreAllFromSession= */ true,
                walkerRequestContextTestWrapper.Get().ItemProxyAdapter());

            wrapper->GetUserInfo().CreateScenarioUserInfoBuilder()->AddProfile(
                "response.user.info", "response_user_info", "some response description");

            wrapper->GetAnalyticsInfo().CreateScenarioAnalyticsInfoBuilder();
            wrappers.emplace_back(std::move(wrapper));
        }

        NMegamind::TMegamindAnalyticsInfo expectedAnalyticsInfo;
        const auto status = google::protobuf::util::JsonStringToMessage(
            TString{MEGAMIND_ANALYTICS_INFO_WITH_EMPTY_WINNER_SCENARIO}, &expectedAnalyticsInfo);
        UNIT_ASSERT(status.ok());

        TCommonScenarioWalker scenarioWalker(walkerRequestContextTestWrapper.Get().GlobalCtx());
        NMegamind::TMegamindAnalyticsInfoBuilder analyticsInfoBuilder;
        scenarioWalker.PreFillAnalyticsInfo(analyticsInfoBuilder, ctx);
        scenarioWalker.PostFillAnalyticsInfo(analyticsInfoBuilder, ctx, wrappers, wrappers.front(),
                                             {VINSLESS_SCENARIO_NAME}, walkerRequestContextTestWrapper.GetRequest());

        UNIT_ASSERT_MESSAGES_EQUAL(expectedAnalyticsInfo, analyticsInfoBuilder.BuildProto());
    }

    Y_UNIT_TEST(FillAnalyticsInfoWithSelectedWrapperAndOnlyTunnellerAnalyticsInfo) {
        const TString UTTERANCE = "utterance";
        const TMaybe<TStringBuf> SHOWN_UTTERANCE;
        const TString VINSLESS_SCENARIO_NAME = "alice.response";

        StrictMock<TMockWalkerRequestContextTestWrapper> walkerRequestContextTestWrapper;
        TMockContext ctx;
        TMockResponses responses;
        TSearchResponse searchObject(TString{MEGAMIND_TUNNELLER_RESPONSE}, "", TRTLogger::NullLogger(),
                                     /* initDataSources= */ false);
        EXPECT_CALL(responses, WebSearchResponse(_)).WillRepeatedly(ReturnRef(searchObject));

        TScenarioConfig vinslessConfig;
        vinslessConfig.SetName(VINSLESS_SCENARIO_NAME);

        TConfigBasedAppHostPureProtocolScenario vinslessScenario(vinslessConfig);

        EXPECT_CALL(ctx, Responses()).WillRepeatedly(ReturnRef(responses));
        EXPECT_CALL(ctx, PolyglotUtterance()).WillRepeatedly(Return(UTTERANCE));
        EXPECT_CALL(ctx, AsrNormalizedUtterance()).WillRepeatedly(Return(SHOWN_UTTERANCE));
        auto skr = TSpeechKitRequestBuilder(TStringBuf("{}")).Build();
        EXPECT_CALL(ctx, SpeechKitRequest()).WillRepeatedly(Return(skr));
        TClientInfo clientInfo{TClientInfoProto{}};
        EXPECT_CALL(ctx, ClientInfo()).WillRepeatedly(ReturnRef(clientInfo));

        TScenarioWrapperPtrs wrappers;
        {
            TScenarioInfraConfig scenarioConfig;
            TMockContext applyContext;
            EXPECT_CALL(applyContext, ScenarioConfig(_)).WillRepeatedly(ReturnRef(scenarioConfig));
            EXPECT_CALL(applyContext, Logger()).WillRepeatedly(ReturnRef(TRTLogger::NullLogger()));
            EXPECT_CALL(applyContext, Sensors())
                .WillRepeatedly(ReturnRef(walkerRequestContextTestWrapper.Get().GlobalCtx().ServiceSensors()));
            EXPECT_CALL(applyContext, ClientInfo()).WillRepeatedly(ReturnRef(clientInfo));
            const IScenarioWrapper::TSemanticFrames& requestFrames = {};
            auto wrapper = MakeIntrusive<TAppHostPureProtocolScenarioWrapper>(
                vinslessScenario, applyContext, requestFrames, NMegamind::TGuidGenerator{},
                EDeferredApplyMode::DeferredCall, /* restoreAllFromSession= */ true,
                walkerRequestContextTestWrapper.Get().ItemProxyAdapter());

            wrapper->GetUserInfo().CreateScenarioUserInfoBuilder()->AddProfile(
                "response.user.info", "response_user_info", "some response description");

            wrapper->GetAnalyticsInfo().CreateScenarioAnalyticsInfoBuilder()->AddTunnellerRawResponse("ABCDEF2");
            wrappers.emplace_back(std::move(wrapper));
        }

        NMegamind::TMegamindAnalyticsInfo expectedAnalyticsInfo;
        const auto status = google::protobuf::util::JsonStringToMessage(
            TString{MEGAMIND_ANALYTICS_INFO_WITH_ONLY_TUNNELLER_WINNER_SCENARIO}, &expectedAnalyticsInfo);
        UNIT_ASSERT(status.ok());

        TCommonScenarioWalker scenarioWalker(walkerRequestContextTestWrapper.Get().GlobalCtx());
        NMegamind::TMegamindAnalyticsInfoBuilder analyticsInfoBuilder;
        scenarioWalker.PreFillAnalyticsInfo(analyticsInfoBuilder, ctx);
        scenarioWalker.PostFillAnalyticsInfo(analyticsInfoBuilder, ctx, wrappers, wrappers.front(),
                                             {VINSLESS_SCENARIO_NAME}, walkerRequestContextTestWrapper.GetRequest());

        UNIT_ASSERT_MESSAGES_EQUAL(expectedAnalyticsInfo, analyticsInfoBuilder.BuildProto());
    }

    Y_UNIT_TEST(FillAnalyticsInfoWithoutSelectedWrapperAndEmptyAnalyticsInfo) {
        const TString UTTERANCE = "utterance";
        const TMaybe<TStringBuf> SHOWN_UTTERANCE = "utterance!";
        const TString VINSLESS_SCENARIO_NAME = "alice.response";

        StrictMock<TMockWalkerRequestContextTestWrapper> walkerRequestContextTestWrapper;
        TMockContext ctx;
        TMockResponses responses;
        TSearchResponse searchObject(TString{MEGAMIND_TUNNELLER_RESPONSE}, "", TRTLogger::NullLogger(),
                                     /* initDataSources= */ false);
        EXPECT_CALL(responses, WebSearchResponse(_)).WillRepeatedly(ReturnRef(searchObject));

        TScenarioConfig vinslessConfig;
        vinslessConfig.SetName(VINSLESS_SCENARIO_NAME);

        TConfigBasedAppHostPureProtocolScenario vinslessScenario(vinslessConfig);

        EXPECT_CALL(ctx, Responses()).WillRepeatedly(ReturnRef(responses));
        EXPECT_CALL(ctx, PolyglotUtterance()).WillRepeatedly(Return(UTTERANCE));
        EXPECT_CALL(ctx, AsrNormalizedUtterance()).WillRepeatedly(Return(SHOWN_UTTERANCE));
        auto skr = TSpeechKitRequestBuilder(TStringBuf("{}")).Build();
        EXPECT_CALL(ctx, SpeechKitRequest()).WillRepeatedly(Return(skr));
        TClientInfo clientInfo{TClientInfoProto{}};
        EXPECT_CALL(ctx, ClientInfo()).WillRepeatedly(ReturnRef(clientInfo));

        TScenarioWrapperPtrs wrappers;
        {
            TScenarioInfraConfig scenarioConfig;
            TMockContext applyContext;
            EXPECT_CALL(applyContext, ScenarioConfig(_)).WillRepeatedly(ReturnRef(scenarioConfig));
            EXPECT_CALL(applyContext, Logger()).WillRepeatedly(ReturnRef(TRTLogger::NullLogger()));
            EXPECT_CALL(applyContext, Sensors())
                .WillRepeatedly(ReturnRef(walkerRequestContextTestWrapper.Get().GlobalCtx().ServiceSensors()));
            EXPECT_CALL(applyContext, ClientInfo()).WillRepeatedly(ReturnRef(clientInfo));
            const IScenarioWrapper::TSemanticFrames& requestFrames = {};
            auto wrapper = MakeIntrusive<TAppHostPureProtocolScenarioWrapper>(
                vinslessScenario, applyContext, requestFrames, NMegamind::TGuidGenerator{},
                EDeferredApplyMode::DeferredCall, /* restoreAllFromSession= */ true,
                walkerRequestContextTestWrapper.Get().ItemProxyAdapter());

            wrapper->GetUserInfo().CreateScenarioUserInfoBuilder()->AddProfile(
                "response.user.info", "response_user_info", "some response description");

            wrappers.emplace_back(std::move(wrapper));
        }

        NMegamind::TMegamindAnalyticsInfo expectedAnalyticsInfo;
        const auto status = google::protobuf::util::JsonStringToMessage(
            TString{MEGAMIND_ANALYTICS_INFO_WITHOUT_EMPTY_WINNER_SCENARIO}, &expectedAnalyticsInfo);
        UNIT_ASSERT(status.ok());

        TCommonScenarioWalker scenarioWalker(walkerRequestContextTestWrapper.Get().GlobalCtx());
        NMegamind::TMegamindAnalyticsInfoBuilder analyticsInfoBuilder;
        scenarioWalker.PreFillAnalyticsInfo(analyticsInfoBuilder, ctx);
        scenarioWalker.PostFillAnalyticsInfo(analyticsInfoBuilder, ctx, wrappers, /* wrapper= */ nullptr,
                                             /* scenariosWithTunnellerResponses= */ {}, walkerRequestContextTestWrapper.GetRequest());

        UNIT_ASSERT_MESSAGES_EQUAL(expectedAnalyticsInfo, analyticsInfoBuilder.BuildProto());
    }

    Y_UNIT_TEST(FillAnalyticsInfoWithScenarioTimings) {
        const TString UTTERANCE = "utterance";
        const TMaybe<TStringBuf> SHOWN_UTTERANCE;
        const TString VINSLESS_SCENARIO_NAME = "alice.response";

        StrictMock<TMockWalkerRequestContextTestWrapper> walkerRequestContextTestWrapper;
        TMockContext ctx;
        TMockResponses responses;
        TSearchResponse searchObject("", "", TRTLogger::NullLogger(),
                                     /* initDataSources= */ false);
        EXPECT_CALL(responses, WebSearchResponse(_)).WillRepeatedly(ReturnRef(searchObject));
        EXPECT_CALL(ctx, Responses()).WillRepeatedly(ReturnRef(responses));

        TScenarioConfig vinslessConfig;
        vinslessConfig.SetName(VINSLESS_SCENARIO_NAME);

        TConfigBasedAppHostPureProtocolScenario vinslessScenario(vinslessConfig);

        EXPECT_CALL(ctx, Responses()).WillRepeatedly(ReturnRef(responses));
        EXPECT_CALL(ctx, PolyglotUtterance()).WillRepeatedly(Return(UTTERANCE));
        EXPECT_CALL(ctx, AsrNormalizedUtterance()).WillRepeatedly(Return(SHOWN_UTTERANCE));
        auto skr = TSpeechKitRequestBuilder(TStringBuf("{}")).Build();
        EXPECT_CALL(ctx, SpeechKitRequest()).WillRepeatedly(Return(skr));
        TClientInfo clientInfo{TClientInfoProto{}};
        EXPECT_CALL(ctx, ClientInfo()).WillRepeatedly(ReturnRef(clientInfo));

        const TInstant startTime = TInstant::ParseIso8601("2020-10-07T12:00:00Z");

        TScenarioWrapperPtrs wrappers;
        {
            TScenarioInfraConfig scenarioConfig;
            TMockContext applyContext;
            EXPECT_CALL(applyContext, ScenarioConfig(_)).WillRepeatedly(ReturnRef(scenarioConfig));
            EXPECT_CALL(applyContext, Logger()).WillRepeatedly(ReturnRef(TRTLogger::NullLogger()));
            EXPECT_CALL(applyContext, Sensors())
                .WillRepeatedly(ReturnRef(walkerRequestContextTestWrapper.Get().GlobalCtx().ServiceSensors()));
            EXPECT_CALL(applyContext, ClientInfo()).WillRepeatedly(ReturnRef(clientInfo));
            const IScenarioWrapper::TSemanticFrames& requestFrames = {};
            auto wrapper = MakeIntrusive<TAppHostPureProtocolScenarioWrapper>(
                vinslessScenario, applyContext, requestFrames, NMegamind::TGuidGenerator{},
                EDeferredApplyMode::DeferredCall, /* restoreAllFromSession= */ true,
                walkerRequestContextTestWrapper.Get().ItemProxyAdapter());

            wrapper->GetAnalyticsInfo()
                .CreateScenarioAnalyticsInfoBuilder()
                ->SetIntentName("alice.response.intent")
                .SetStageStartTime("run", startTime + TDuration::MilliSeconds(5))
                .AddSourceResponseDuration("run", "protocol-run", TDuration::MilliSeconds(50))
                .SetStageStartTime("commit", startTime + TDuration::MilliSeconds(150))
                .AddSourceResponseDuration("commit", "protocol-commit", TDuration::MilliSeconds(100));

            wrappers.emplace_back(std::move(wrapper));
        }

        NMegamind::TMegamindAnalyticsInfo expectedAnalyticsInfo;
        const auto status = google::protobuf::util::JsonStringToMessage(
            TString{MEGAMIND_ANALYTICS_INFO_WITH_SCENARIO_TIMINGS}, &expectedAnalyticsInfo);
        UNIT_ASSERT(status.ok());

        TCommonScenarioWalker scenarioWalker(walkerRequestContextTestWrapper.Get().GlobalCtx());
        NMegamind::TMegamindAnalyticsInfoBuilder analyticsInfoBuilder;
        scenarioWalker.PreFillAnalyticsInfo(analyticsInfoBuilder, ctx);
        scenarioWalker.PostFillAnalyticsInfo(analyticsInfoBuilder, ctx, wrappers, wrappers.front(),
                                             {NImpl::ALL_SCENARIOS}, walkerRequestContextTestWrapper.GetRequest());

        UNIT_ASSERT_MESSAGES_EQUAL(expectedAnalyticsInfo, analyticsInfoBuilder.BuildProto());
    }

    Y_UNIT_TEST(FillAnalyticsInfoWithParentProductScenarioName) {
        const TString UTTERANCE = "utterance";
        const TMaybe<TStringBuf> SHOWN_UTTERANCE;
        const TString VINSLESS_SCENARIO_NAME = "alice.response";
        const TString VINSLESS_TUNNELLER_RESPONSE = "Response tunneller raws";

        StrictMock<TMockWalkerRequestContextTestWrapper> walkerRequestContextTestWrapper;
        TMockContext ctx;
        TMockResponses responses;
        TSearchResponse searchObject(TString{MEGAMIND_TUNNELLER_RESPONSE}, "", TRTLogger::NullLogger(),
                                     /* initDataSources= */ false);
        EXPECT_CALL(responses, WebSearchResponse(_)).WillRepeatedly(ReturnRef(searchObject));

        TScenarioConfig vinslessConfig;
        vinslessConfig.SetName(VINSLESS_SCENARIO_NAME);

        TConfigBasedAppHostPureProtocolScenario vinslessScenario(vinslessConfig);

        EXPECT_CALL(ctx, Responses()).WillRepeatedly(ReturnRef(responses));
        EXPECT_CALL(ctx, PolyglotUtterance()).WillRepeatedly(Return(UTTERANCE));
        EXPECT_CALL(ctx, AsrNormalizedUtterance()).WillRepeatedly(Return(SHOWN_UTTERANCE));
        auto skr = TSpeechKitRequestBuilder(TStringBuf("{}")).Build();
        EXPECT_CALL(ctx, SpeechKitRequest()).WillRepeatedly(Return(skr));
        TClientInfo clientInfo{TClientInfoProto{}};
        EXPECT_CALL(ctx, ClientInfo()).WillRepeatedly(ReturnRef(clientInfo));

        TScenarioWrapperPtrs wrappers;
        {
            TScenarioInfraConfig scenarioConfig;
            TMockContext applyContext;
            EXPECT_CALL(applyContext, ScenarioConfig(_)).WillRepeatedly(ReturnRef(scenarioConfig));
            EXPECT_CALL(applyContext, Logger()).WillRepeatedly(ReturnRef(TRTLogger::NullLogger()));
            EXPECT_CALL(applyContext, Sensors())
                .WillRepeatedly(ReturnRef(walkerRequestContextTestWrapper.Get().GlobalCtx().ServiceSensors()));
            EXPECT_CALL(applyContext, ClientInfo()).WillRepeatedly(ReturnRef(clientInfo));
            const IScenarioWrapper::TSemanticFrames& requestFrames = {};
            auto wrapper = MakeIntrusive<TAppHostPureProtocolScenarioWrapper>(
                vinslessScenario, applyContext, requestFrames, NMegamind::TGuidGenerator{},
                EDeferredApplyMode::DeferredCall, /* restoreAllFromSession= */ true,
                walkerRequestContextTestWrapper.Get().ItemProxyAdapter());

            wrapper->GetUserInfo().CreateScenarioUserInfoBuilder()->AddProfile(
                "response.user.info", "response_user_info", "some response description");
            wrapper->GetAnalyticsInfo()
                .CreateScenarioAnalyticsInfoBuilder()
                ->SetIntentName("alice.response.intent")
                .AddObject("response.object.id", "response_some_object", "yet another response description")
                .AddTunnellerRawResponse(VINSLESS_TUNNELLER_RESPONSE);

            wrapper->GetAnalyticsInfo().SetParentProductScenarioName("parent_product_scenario");

            wrappers.emplace_back(std::move(wrapper));
        }

        NMegamind::TMegamindAnalyticsInfo expectedAnalyticsInfo;
        const auto status = google::protobuf::util::JsonStringToMessage(
            TString{MEGAMIND_ANALYTICS_INFO_WITH_PARENT_PRODUCT_SCENARIO_NAME}, &expectedAnalyticsInfo);
        UNIT_ASSERT(status.ok());

        TCommonScenarioWalker scenarioWalker(walkerRequestContextTestWrapper.Get().GlobalCtx());
        NMegamind::TMegamindAnalyticsInfoBuilder analyticsInfoBuilder;
        analyticsInfoBuilder.SetParentProductScenarioName("parent_product_scenario");

        scenarioWalker.PreFillAnalyticsInfo(analyticsInfoBuilder, ctx);
        scenarioWalker.PostFillAnalyticsInfo(analyticsInfoBuilder, ctx, wrappers, wrappers.front(),
                                             {NImpl::ALL_SCENARIOS}, walkerRequestContextTestWrapper.GetRequest());

        UNIT_ASSERT_MESSAGES_EQUAL(expectedAnalyticsInfo, analyticsInfoBuilder.BuildProto());
    }

    Y_UNIT_TEST(SortWrappers_givenProtocolScenarioRequiringNoWebSearch_placesItAccordingToAlphabeticalOrder) {
        TScenarioWrapperPtrs wrappers{
            CreateProtocolScenarioWrapper("c", Nothing()),
            CreateProtocolScenarioWrapper("b", Nothing())
        };
        IContext::TExpFlags expFlags;
        NImpl::SortWrappers(wrappers, expFlags);
        UNIT_ASSERT_VALUES_EQUAL(2, wrappers.size());
        UNIT_ASSERT_VALUES_EQUAL("b", wrappers[0]->GetScenario().GetName());
        UNIT_ASSERT_VALUES_EQUAL("c", wrappers[1]->GetScenario().GetName());
    }

    Y_UNIT_TEST(SortWrappers_givenProtocolScenarioRequiringWebSearch_makesItLast) {
        TScenarioWrapperPtrs wrappers{
            CreateProtocolScenarioWrapper("c", Nothing()),
            CreateProtocolScenarioWrapper("b", EDataSourceType::WEB_SEARCH_DOCS)
        };
        IContext::TExpFlags expFlags;
        NImpl::SortWrappers(wrappers, expFlags);
        UNIT_ASSERT_VALUES_EQUAL(2, wrappers.size());
        UNIT_ASSERT_VALUES_EQUAL("c", wrappers[0]->GetScenario().GetName());
        UNIT_ASSERT_VALUES_EQUAL("b", wrappers[1]->GetScenario().GetName());
    }

    Y_UNIT_TEST(SortWrappers_givenProtocolScenarioRequiringWebSearchRenderrer_makesItLast) {
        TScenarioWrapperPtrs wrappers{
            CreateProtocolScenarioWrapper("c", Nothing()),
            CreateProtocolScenarioWrapper("b", EDataSourceType::WEB_SEARCH_RENDERRER)
        };
        IContext::TExpFlags expFlags;
        NImpl::SortWrappers(wrappers, expFlags);
        UNIT_ASSERT_VALUES_EQUAL(2, wrappers.size());
        UNIT_ASSERT_VALUES_EQUAL("c", wrappers[0]->GetScenario().GetName());
        UNIT_ASSERT_VALUES_EQUAL("b", wrappers[1]->GetScenario().GetName());
    }

    Y_UNIT_TEST(SortWrappers_givenProtocolScenarioRequiringWebSearchRenderrerUnderFlagWithFlag_makesItLast) {
        TScenarioWrapperPtrs wrappers{
            CreateProtocolScenarioWrapper("c", Nothing()),
            CreateProtocolScenarioWrapper("b", EDataSourceType::WEB_SEARCH_RENDERRER)
        };
        IContext::TExpFlags expFlags = {{ToString(EXP_SET_SCENARIO_DATASOURCE_PREFIX) + "b:WEB_SEARCH_RENDERRER", "1"}};
        NImpl::SortWrappers(wrappers, expFlags);
        UNIT_ASSERT_VALUES_EQUAL(2, wrappers.size());
        UNIT_ASSERT_VALUES_EQUAL("c", wrappers[0]->GetScenario().GetName());
        UNIT_ASSERT_VALUES_EQUAL("b", wrappers[1]->GetScenario().GetName());
    }


    Y_UNIT_TEST(RaiseErrorOnFailedScenarios) {
        TVector<THolder<TMockScenario>> scenarios;
        TScenariosErrors errors;

        const TString NOTHING("nothing");
        const TString E_512("512");
        const TString ERROR_LOGIC("error_logic");
        const TString ERROR_LOGIC_AND_512("error_logic_and_512");

        scenarios.push_back(MakeHolder<TMockScenario>(NOTHING, false));

        scenarios.push_back(MakeHolder<TMockScenario>(E_512, false));

        scenarios.push_back(MakeHolder<TMockScenario>(ERROR_LOGIC, false));
        errors.Add(scenarios.back()->GetName(), NImpl::STAGE_ASK, TError(TError::EType::Logic));

        scenarios.push_back(MakeHolder<TMockScenario>(ERROR_LOGIC_AND_512, false));
        errors.Add(scenarios.back()->GetName(), NImpl::STAGE_ASK, TError(TError::EType::Logic));

        TVector<TScenarioResponse> scenarioResponses;
        scenarioResponses.push_back(TScenarioResponse(/* scenarioName= */ NOTHING, /* requestFrames= */ {},
                                                      /* scenarioAcceptsAnyInput= */ true));
        scenarioResponses.push_back(TScenarioResponse(/* scenarioName= */ E_512, /* requestFrames= */ {},
                                                      /* scenarioAcceptsAnyInput= */ true));
        scenarioResponses.back().SetHttpCode(HTTP_UNASSIGNED_512);
        scenarioResponses.back().SetHttpCode(HTTP_UNASSIGNED_512);
        scenarioResponses.push_back(TScenarioResponse(/* scenarioName= */ ERROR_LOGIC,
                                                      /* requestFrames= */ {},
                                                      /* scenarioAcceptsAnyInput= */ true));
        scenarioResponses.push_back(TScenarioResponse(/* scenarioName= */ ERROR_LOGIC_AND_512,
                                                      /* requestFrames= */ {}, /* scenarioAcceptsAnyInput= */ true));
        scenarioResponses.back().SetHttpCode(HTTP_UNASSIGNED_512);

        for (const auto scenarioNames :
             {TStringBuf("nothing"), TStringBuf("absent"), TStringBuf("absent1;absent2")}) {
            UNIT_ASSERT_C(
                !NImpl::RaiseErrorOnFailedScenarios(errors, scenarioResponses, scenarioNames, TRTLogger::NullLogger()),
                TStringBuilder{} << "Scenarios: " << scenarioNames << " must not raise error");
        }

        for (const auto scenarioNames :
             {TStringBuf("512"), TStringBuf("error_logic"), TStringBuf("error_logic_and_512"),
              TStringBuf("nothing;error_logic"), TStringBuf("nothing;512"),
              TStringBuf("absent;512"), TStringBuf("absent;error_logic"),
              TStringBuf{NImpl::ALL_SCENARIOS}}) {
            UNIT_ASSERT_C(
                NImpl::RaiseErrorOnFailedScenarios(errors, scenarioResponses, scenarioNames, TRTLogger::NullLogger()),
                TStringBuilder{} << "Scenarios: " << scenarioNames << " must raise error");
        }
    }

    Y_UNIT_TEST(AddSearchRelatedScenarioStats) {
        using EEvent = TCommonScenarioWalker::EHeavyScenarioEvent;

        auto scenarioSearch = MakeSimpleShared<NiceMock<TMockScenario>>("Search", /* needWebSearch= */ true);
        auto scenarioNonSearch = MakeSimpleShared<NiceMock<TMockScenario>>("NonSearch", /* needWebSearch= */ false);

        auto searchScenarioWrapper = MakeIntrusive<NiceMock<TMockScenarioWrapper>>();
        EXPECT_CALL(*searchScenarioWrapper, GetScenario())
            .WillRepeatedly(
                Invoke([scenario = std::move(scenarioSearch)]() -> const TScenario& { return *scenario; }));
        EXPECT_CALL(*searchScenarioWrapper, IsSuccess()).WillRepeatedly(Return(true));

        auto nonSearchScenarioWrapper = MakeIntrusive<NiceMock<TMockScenarioWrapper>>();
        EXPECT_CALL(*nonSearchScenarioWrapper, GetScenario())
            .WillRepeatedly(
                Invoke([scenario = std::move(scenarioNonSearch)]() -> const TScenario& { return *scenario; }));
        EXPECT_CALL(*nonSearchScenarioWrapper, IsSuccess()).WillRepeatedly(Return(true));

        const auto skr = TSpeechKitRequestBuilder(GOOD_QUASAR_REQ).Build();

        {
            TMockContext ctx;
            EXPECT_CALL(ctx, SpeechKitRequest()).WillOnce(Return(skr));

            StrictMock<TMockSensors> sensors;
            EXPECT_CALL(sensors, IncRate(NMonitoring::TLabels{{"name", "scenario.with_search_per_second"},
                                                              {"client_type", "smart_speaker"},
                                                              {"event", "request"},
                                                              {"scenario_name", "Search"}}));

            EXPECT_CALL(ctx, Sensors()).WillOnce(ReturnRef(sensors));
            NImpl::AddSearchRelatedScenarioStats(ctx, searchScenarioWrapper, EEvent::Request);
        }
        {
            TMockContext ctx;
            NMegamind::TClassificationConfig classificationConfigEmpty;
            EXPECT_CALL(ctx, ClassificationConfig()).WillOnce(ReturnRef(classificationConfigEmpty));
            IContext::TExpFlags expFlags;
            EXPECT_CALL(ctx, ExpFlags()).WillOnce(ReturnRef(expFlags));

            // No error due to lack of mocking should fire.
            NImpl::AddSearchRelatedScenarioStats(ctx, nonSearchScenarioWrapper, EEvent::Request);
        }
        {
            TMockContext ctx;
            EXPECT_CALL(ctx, SpeechKitRequest()).WillOnce(Return(skr));
            IContext::TExpFlags expFlags;
            EXPECT_CALL(ctx, ExpFlags()).WillOnce(ReturnRef(expFlags));

            NMegamind::TClassificationConfig classificationConfig;
            (*classificationConfig
                  .MutableScenarioClassificationConfigs())[nonSearchScenarioWrapper->GetScenario().GetName()]
                .SetUseFormulasForRanking(true);
            EXPECT_CALL(ctx, ClassificationConfig()).WillOnce(ReturnRef(classificationConfig));

            StrictMock<TMockSensors> sensors;
            EXPECT_CALL(sensors, IncRate(NMonitoring::TLabels{{"name", "scenario.with_search_per_second"},
                                                              {"client_type", "smart_speaker"},
                                                              {"event", "win"},
                                                              {"scenario_name", "NonSearch"}}));

            EXPECT_CALL(ctx, Sensors()).WillOnce(ReturnRef(sensors));
            NImpl::AddSearchRelatedScenarioStats(ctx, nonSearchScenarioWrapper, EEvent::Win);
        }
    }

    Y_UNIT_TEST(SkipWebSearch) {
        NMegamind::TClassificationConfig classificationConfigEmpty;
        {
            TMockContext ctx;
            EXPECT_CALL(ctx, ClassificationConfig()).WillRepeatedly(ReturnRef(classificationConfigEmpty));
            IContext::TExpFlags expFlags;
            EXPECT_CALL(ctx, ExpFlags()).WillRepeatedly(ReturnRef(expFlags));

            TMockScenario scenarioX("X", /* needWebSearch= */ false);
            TMockScenario scenarioY("Y", /* needWebSearch= */  false);
            TScenarioToRequestFrames scenarioToRequestFrames;
            scenarioToRequestFrames[CreateScenarioRef(scenarioX)] = {};
            scenarioToRequestFrames[CreateScenarioRef(scenarioY)] = {};
            NAlice::TConfig_TScenarios_TConfig scenarioConfig;
            EXPECT_CALL(ctx, ScenarioConfig(_)).WillRepeatedly(ReturnRef(scenarioConfig));
            UNIT_ASSERT(!NImpl::ShouldMakeSearchRequest(scenarioToRequestFrames, ctx));
        }
        {
            TMockContext ctx;
            EXPECT_CALL(ctx, ClassificationConfig()).WillRepeatedly(ReturnRef(classificationConfigEmpty));
            IContext::TExpFlags expFlags = {{ToString(EXP_SET_SCENARIO_DATASOURCE_PREFIX) + "X:WEB_SEARCH_RENDERRER", "1"}};
            EXPECT_CALL(ctx, ExpFlags()).WillRepeatedly(ReturnRef(expFlags));

            TMockScenario scenarioX("X", /* needWebSearch= */ false);
            TMockScenario scenarioY("Y", /* needWebSearch= */  false);
            TScenarioToRequestFrames scenarioToRequestFrames;
            scenarioToRequestFrames[CreateScenarioRef(scenarioX)] = {};
            scenarioToRequestFrames[CreateScenarioRef(scenarioY)] = {};
            NAlice::TConfig_TScenarios_TConfig scenarioConfig;
            EXPECT_CALL(ctx, ScenarioConfig(_)).WillRepeatedly(ReturnRef(scenarioConfig));
            UNIT_ASSERT(NImpl::ShouldMakeSearchRequest(scenarioToRequestFrames, ctx));
        }
        {
            TMockContext ctx;
            EXPECT_CALL(ctx, ClassificationConfig()).WillRepeatedly(ReturnRef(classificationConfigEmpty));
            IContext::TExpFlags expFlags;
            EXPECT_CALL(ctx, ExpFlags()).WillRepeatedly(ReturnRef(expFlags));

            TMockScenario scenarioX("X", /* needWebSearch= */  true);
            TMockScenario scenarioY("Y", /* needWebSearch= */  false);
            TScenarioToRequestFrames scenarioToRequestFrames;
            scenarioToRequestFrames[CreateScenarioRef(scenarioX)] = {};
            scenarioToRequestFrames[CreateScenarioRef(scenarioY)] = {};
            NAlice::TConfig_TScenarios_TConfig scenarioConfig;
            EXPECT_CALL(ctx, ScenarioConfig(_)).WillRepeatedly(ReturnRef(scenarioConfig));
            UNIT_ASSERT(NImpl::ShouldMakeSearchRequest(scenarioToRequestFrames, ctx));
        }
        {
            TMockContext ctx;
            NMegamind::TClassificationConfig classificationConfig;
            (*classificationConfig.MutableScenarioClassificationConfigs())[TString{"X"}].SetUseFormulasForRanking(true);
            EXPECT_CALL(ctx, ClassificationConfig()).WillRepeatedly(ReturnRef(classificationConfig));
            IContext::TExpFlags expFlags;
            EXPECT_CALL(ctx, ExpFlags()).WillRepeatedly(ReturnRef(expFlags));

            TScenarioToRequestFrames scenarioToRequestFrames;
            TMockScenario scenarioX("X", /* needWebSearch= */  false);
            TMockScenario scenarioY("Y", /* needWebSearch= */  false);
            scenarioToRequestFrames[CreateScenarioRef(scenarioX)] = {};
            scenarioToRequestFrames[CreateScenarioRef(scenarioY)] = {};
            NAlice::TConfig_TScenarios_TConfig scenarioConfig;
            EXPECT_CALL(ctx, ScenarioConfig(_)).WillRepeatedly(ReturnRef(scenarioConfig));
            UNIT_ASSERT(!NImpl::ShouldMakeSearchRequest(scenarioToRequestFrames, ctx));
        }
        {
            TMockContext ctx;
            NMegamind::TClassificationConfig classificationConfig;
            (*classificationConfig.MutableScenarioClassificationConfigs())[TString{"X"}].SetUseFormulasForRanking(true);
            (*classificationConfig.MutableScenarioClassificationConfigs())[TString{"Y"}].SetUseFormulasForRanking(true);
            EXPECT_CALL(ctx, ClassificationConfig()).WillRepeatedly(ReturnRef(classificationConfig));
            IContext::TExpFlags expFlags;
            EXPECT_CALL(ctx, ExpFlags()).WillRepeatedly(ReturnRef(expFlags));

            TScenarioToRequestFrames scenarioToRequestFrames;
            TMockScenario scenarioX("X", /* needWebSearch= */  false);
            TMockScenario scenarioY("Y", /* needWebSearch= */  false);
            scenarioToRequestFrames[CreateScenarioRef(scenarioX)] = {};
            scenarioToRequestFrames[CreateScenarioRef(scenarioY)] = {};
            NAlice::TConfig_TScenarios_TConfig scenarioConfig;
            EXPECT_CALL(ctx, ScenarioConfig(_)).WillRepeatedly(ReturnRef(scenarioConfig));
            UNIT_ASSERT(NImpl::ShouldMakeSearchRequest(scenarioToRequestFrames, ctx));
        }
        {
            TMockContext ctx;
            NMegamind::TClassificationConfig classificationConfig;
            (*classificationConfig.MutableScenarioClassificationConfigs())[TString{"X"}].SetUseFormulasForRanking(true);
            EXPECT_CALL(ctx, ClassificationConfig()).WillRepeatedly(ReturnRef(classificationConfig));
            IContext::TExpFlags expFlags;
            EXPECT_CALL(ctx, ExpFlags()).WillOnce(ReturnRef(expFlags));

            TScenarioToRequestFrames scenarioToRequestFrames;
            TMockScenario scenarioX("X", /* needWebSearch= */  false);
            scenarioToRequestFrames[CreateScenarioRef(scenarioX)] = {};
            NAlice::TConfig_TScenarios_TConfig scenarioConfig;
            EXPECT_CALL(ctx, ScenarioConfig(_)).WillRepeatedly(ReturnRef(scenarioConfig));
            UNIT_ASSERT(!NImpl::ShouldMakeSearchRequest(scenarioToRequestFrames, ctx));
        }
        {
            TMockContext ctx;
            EXPECT_CALL(ctx, ClassificationConfig()).WillRepeatedly(ReturnRef(classificationConfigEmpty));
            TScenarioToRequestFrames scenarioToRequestFrames;
            TMockScenario scenarioX("X", /* needWebSearch= */  true);
            scenarioToRequestFrames[CreateScenarioRef(scenarioX)] = {};
            NAlice::TConfig_TScenarios_TConfig scenarioConfig;
            EXPECT_CALL(ctx, ScenarioConfig(_)).WillRepeatedly(ReturnRef(scenarioConfig));
            UNIT_ASSERT(NImpl::ShouldMakeSearchRequest(scenarioToRequestFrames, ctx));
        }
        {
            TMockContext ctx;
            EXPECT_CALL(ctx, ClassificationConfig()).WillRepeatedly(ReturnRef(classificationConfigEmpty));
            IContext::TExpFlags expFlags;
            EXPECT_CALL(ctx, ExpFlags()).WillRepeatedly(ReturnRef(expFlags));

            TMockScenario scenarioX("X", /* needWebSearch= */ false);
            const auto targetFrameName = "some_frame";
            NAlice::TConfig_TScenarios_TConfig scenarioConfig;
            auto& cds = *scenarioConfig.AddConditionalDataSources();
            cds.SetDataSourceType(EDataSourceType::WEB_SEARCH_DOCS);
            cds.AddConditions()->MutableOnSemanticFrameCondition()->SetSemanticFrameName(targetFrameName);
            EXPECT_CALL(ctx, ScenarioConfig(_)).WillRepeatedly(ReturnRef(scenarioConfig));
            TSemanticFrame frame;
            {
                frame.SetName(targetFrameName);
                TScenarioToRequestFrames scenarioToRequestFrames;
                scenarioToRequestFrames[CreateScenarioRef(scenarioX)] = {frame};
                UNIT_ASSERT(NImpl::ShouldMakeSearchRequest(scenarioToRequestFrames, ctx));
            }
            {
                frame.SetName("another_frame");
                TScenarioToRequestFrames scenarioToRequestFrames;
                scenarioToRequestFrames[CreateScenarioRef(scenarioX)] = {frame};
                UNIT_ASSERT(!NImpl::ShouldMakeSearchRequest(scenarioToRequestFrames, ctx));
            }
            {
                frame.SetName(targetFrameName);
                TScenarioToRequestFrames scenarioToRequestFrames;
                scenarioToRequestFrames[CreateScenarioRef(scenarioX)] = {frame};

                NAlice::TConfig_TScenarios_TConfig scenarioConfig;
                auto& cds = *scenarioConfig.AddConditionalDataSources();
                cds.SetDataSourceType(EDataSourceType::BLACK_BOX);
                cds.AddConditions()->MutableOnSemanticFrameCondition()->SetSemanticFrameName(targetFrameName);
                EXPECT_CALL(ctx, ScenarioConfig(_)).WillRepeatedly(ReturnRef(scenarioConfig));
                UNIT_ASSERT(!NImpl::ShouldMakeSearchRequest(scenarioToRequestFrames, ctx));
            }
        }
        {
            TMockContext ctx;
            EXPECT_CALL(ctx, ClassificationConfig()).WillRepeatedly(ReturnRef(classificationConfigEmpty));
            IContext::TExpFlags expFlags;
            EXPECT_CALL(ctx, ExpFlags()).WillRepeatedly(ReturnRef(expFlags));

            TMockScenario scenarioX("X", /* needWebSearch= */ false);
            NAlice::TConfig_TScenarios_TConfig scenarioConfig;
            auto& cds = *scenarioConfig.AddConditionalDataSources();
            cds.SetDataSourceType(EDataSourceType::WEB_SEARCH_DOCS);
            cds.AddConditions()->MutableOnUserLanguage()->SetLanguage(ELang::L_ARA);
            cds.AddConditions()->MutableOnExperimentFlag()->SetFlagName("exp_flag_name");
            EXPECT_CALL(ctx, ScenarioConfig(_)).WillRepeatedly(ReturnRef(scenarioConfig));
            EXPECT_CALL(ctx, HasExpFlag("exp_flag_name")).WillRepeatedly(Return(false));

            TScenarioToRequestFrames scenarioToRequestFrames;
            TSemanticFrame frame;
            frame.SetName("any_frame");
            scenarioToRequestFrames[CreateScenarioRef(scenarioX)] = {frame};

            {
                EXPECT_CALL(ctx, Language()).WillRepeatedly(Return(ELanguage::LANG_RUS));
                EXPECT_CALL(ctx, HasExpFlag("exp_flag_name")).WillRepeatedly(Return(true));
                UNIT_ASSERT(NImpl::ShouldMakeSearchRequest(scenarioToRequestFrames, ctx));
                EXPECT_CALL(ctx, HasExpFlag("exp_flag_name")).WillRepeatedly(Return(false));
            }
            {
                EXPECT_CALL(ctx, Language()).WillRepeatedly(Return(ELanguage::LANG_RUS));
                UNIT_ASSERT(!NImpl::ShouldMakeSearchRequest(scenarioToRequestFrames, ctx));
            }
            {
                EXPECT_CALL(ctx, Language()).WillRepeatedly(Return(ELanguage::LANG_ARA));
                UNIT_ASSERT(NImpl::ShouldMakeSearchRequest(scenarioToRequestFrames, ctx));
            }
        }
    }

    Y_UNIT_TEST(CopyScenarioErrorsToWinnerResponse) {
        TWalkerResponse walkerResponse;
        TMockScenario scenarioX("X", /* needWebSearch= */ false);
        TMockScenario scenarioY("Y", /* needWebSearch= */ false);
        auto buildErrorJson = [](TStringBuf scenarioName, TStringBuf stage, TError::EType type) {
            TString text = Sprintf(R"({"error_type":"%s","type":"error",
                                   "message":"{\"scenario_name\":\"%s\",\"message\":\"%s failed\",\"stage\":\"%s\"}"}
                                   )", ToString(type).data(), scenarioName.data(), scenarioName.data(), stage.data());
            return NJson::ReadJsonFastTree(text);
        };
        auto xMeta = buildErrorJson("X", NImpl::STAGE_INIT, TError::EType::Logic);
        auto yMeta = buildErrorJson("Y", NImpl::STAGE_ASK, TError::EType::Http);

        walkerResponse.AddScenarioError(scenarioX.GetName(), NImpl::STAGE_INIT, TError{TError::EType::Logic} << "X failed");
        walkerResponse.AddScenarioError(scenarioY.GetName(), NImpl::STAGE_ASK, TError{} << "Y failed");
        auto skr = TSpeechKitRequestBuilder(TStringBuf(R"({
            "request": {
                "test_ids": [
                    111,
                    222
                ],
                "event": {
                    "type": "text_input"
                }
            }
        })")).Build();
        auto request = CreateRequest(IEvent::CreateEvent(skr.Event()), skr);

        TMockContext ctx;
        EXPECT_CALL(ctx, SpeechKitRequest()).WillRepeatedly(Return(skr));

        StrictMock<TMockSensors> sensors{};
        EXPECT_CALL(ctx, Sensors()).WillRepeatedly(ReturnRef(sensors));
        const auto setSensorsExpects = [&sensors]() {
            EXPECT_CALL(sensors, IncRate(NMonitoring::TLabels{{"test_id", "111"},
                                                              {"name", "test_ids.errors_per_second"},
                                                              {"error_type", "scenario_error"}})).Times(2);
            EXPECT_CALL(sensors, IncRate(NMonitoring::TLabels{{"test_id", "222"},
                                                              {"name", "test_ids.errors_per_second"},
                                                              {"error_type", "scenario_error"}})).Times(2);
        };

        {
            TScenarioResponse topResponse{/* scenarioName= */ "alice.winner.vinsless", /* requestFrames= */ {},
                                          /* scenarioAcceptsAnyInput= */ true};
            TResponseBuilderProto proto;
            proto.MutableResponse();
            auto& builder = topResponse.ForceBuilder(skr, request, NMegamind::TGuidGenerator{}, std::move(proto));
            setSensorsExpects();
            NImpl::CopyScenarioErrorsToWinnerResponse(walkerResponse, topResponse, ctx);
            auto skResponse = builder.GetSKRProto();
            const auto responseJson = SpeechKitResponseToJson(skResponse);
            const auto& meta = responseJson["response"]["meta"].GetArray();
            UNIT_ASSERT_VALUES_EQUAL(meta.size(), 2); // Due to empty analytics info meta added.
            // The order of errors is not deterministic.
            UNIT_ASSERT(IsIn(meta, xMeta));
            UNIT_ASSERT(IsIn(meta, yMeta));
        }
    }

    Y_UNIT_TEST(TCommonScenarioWalker_RegisterScenarios) {
        NiceMock<TMockGlobalContext> globalCtx{};
        EXPECT_CALL(globalCtx, BaseLogger()).WillRepeatedly(ReturnRef(TRTLogger::NullLogger()));

        const TString protocolScenarioName = "protocol.scenario.name";
        const TString dummyScenarioName = "dummy.scenario.name";
        const TString acceptedFrame = "frame";

        TScenarioConfigRegistry scenarioConfigRegistry{};
        scenarioConfigRegistry.AddScenarioConfig([&] {
            TScenarioConfig scenarioConfig{};
            scenarioConfig.SetName(protocolScenarioName);
            scenarioConfig.AddAcceptedFrames(acceptedFrame);
            scenarioConfig.MutableHandlers()->SetRequestType(NAlice::ERequestType::AppHostPure);
            return scenarioConfig;
        }());
        scenarioConfigRegistry.AddScenarioConfig([&] {
            TScenarioConfig scenarioConfig{};
            scenarioConfig.SetName(dummyScenarioName);
            scenarioConfig.MutableHandlers()->SetRequestType(NAlice::ERequestType::AppHostPure);
            return scenarioConfig;
        }());
        EXPECT_CALL(globalCtx, ScenarioConfigRegistry()).WillRepeatedly(ReturnRef(scenarioConfigRegistry));

        StrictMock<TMockSensors> sensors{};
        EXPECT_CALL(globalCtx, ServiceSensors()).WillRepeatedly(ReturnRef(sensors));

        TCommonScenarioWalker walker{globalCtx};

        const auto& scenarios = walker.ScenarioRegistry.GetScenarioRefs();
        THashSet<TString> scenariosPool{
            protocolScenarioName,
            dummyScenarioName,
        };
        for (const auto& scenario : scenarios) {
            scenariosPool.erase(scenario->GetScenario().GetName());
        }
        UNIT_ASSERT(scenariosPool.empty());
    }

    Y_UNIT_TEST(TCommonScenarioWalker_Run) {
        TMockGlobalContextTestWrapper globalContextTestWrapper;

        TConfig config = CreateTestConfig(/* port=*/ 12345);

        TScenarioConfigRegistry scenarioConfigRegistry{};

        NiceMock<TMockSensors> sensors{};

        TFakeStorage storage{};
        TFormulasDescription formulasDescription{};

        TFormulasStorage formulasStorage{storage, formulasDescription};

        TRng rng{4};
        const auto nlgRenderer = NNlg::CreateNlgRendererFromRegisterFunction([](NAlice::NNlg::TEnvironment&){}, rng);

        NiceMock<TMockGlobalContext> globalCtx{};
        EXPECT_CALL(globalCtx, Config()).WillRepeatedly(ReturnRef(config));
        EXPECT_CALL(globalCtx, GetFormulasStorage()).WillRepeatedly(ReturnRef(formulasStorage));
        EXPECT_CALL(globalCtx, ServiceSensors()).WillRepeatedly(ReturnRef(sensors));
        EXPECT_CALL(globalCtx, ScenarioConfigRegistry()).WillRepeatedly(ReturnRef(scenarioConfigRegistry));
        EXPECT_CALL(globalCtx, GetNlgRenderer()).WillRepeatedly(ReturnRef(*nlgRenderer));
        EXPECT_CALL(globalCtx, GeobaseLookup())
            .WillRepeatedly(ReturnRef(globalContextTestWrapper.Get().GeobaseLookup()));

        TClientInfo clientInfo{TClientInfoProto{}};

        TClientFeatures clientFeatures{TClientInfoProto{}, {}};

        auto factorStorage = NMegamind::CreateFactorStorage(NMegamind::CreateFactorDomain());

        TMaybe<TString> boostedScenario = Nothing();

        NMegamind::TClassificationConfig classificationConfigEmpty{};

        NiceMock<TMockContext> context{};
        EXPECT_CALL(context, ClassificationConfig()).WillRepeatedly(ReturnRef(classificationConfigEmpty));
        EXPECT_CALL(context, ClientFeatures()).WillRepeatedly(ReturnRef(clientFeatures));
        EXPECT_CALL(context, ClientInfo()).WillRepeatedly(ReturnRef(clientInfo));
        EXPECT_CALL(context, HasExpFlag(_)).WillRepeatedly(Return(true));
        EXPECT_CALL(context, Language()).WillRepeatedly(Return(ELanguage::LANG_RUS));
        EXPECT_CALL(context, Logger()).WillRepeatedly(ReturnRef(TRTLogger::StderrLogger()));
        EXPECT_CALL(context, Geobase()).WillRepeatedly(ReturnRef(globalContextTestWrapper.Get().GeobaseLookup()));

        TThreadPool scenarioThreads;
        scenarioThreads.Start(/* numThreads= */ 2);
        EXPECT_CALL(context, RequestThreads()).WillRepeatedly(ReturnRef(scenarioThreads));

        NiceMock<NMegamind::TMockInitializer> initializer;

        NiceMock<NMegamind::TMockRequestCtx> requestCtx{&globalCtx, &initializer};

        StrictMock<TMockWalkerRequestContextTestWrapper> walkerRequestContextTestWrapper;

        NiceMock<TMockRunWalkerRequestCtx> walkerCtx{};
        EXPECT_CALL(walkerCtx, Ctx()).WillRepeatedly(ReturnRef(context));
        EXPECT_CALL(walkerCtx, FactorStorage()).WillRepeatedly(ReturnRef(factorStorage));
        EXPECT_CALL(walkerCtx, RequestCtx()).WillRepeatedly(ReturnRef(requestCtx));
        EXPECT_CALL(walkerCtx, Rng()).WillRepeatedly(ReturnRef(rng));
        auto ahCtx = NAlice::NMegamind::NTesting::TTestAppHostCtx{globalCtx};
        NMegamind::TItemProxyAdapter& itemProxyAdapter = ahCtx.ItemProxyAdapter();
        EXPECT_CALL(walkerCtx, ItemProxyAdapter()).WillRepeatedly(ReturnRef(itemProxyAdapter));
        EXPECT_CALL(walkerCtx, PostClassifyState())
            .WillRepeatedly(ReturnRef(walkerRequestContextTestWrapper.GetPostClassifyState()));

        NKvSaaS::TTokensStatsResponse tokensStatsResponse{};
        NScenarios::TSkillDiscoverySaasCandidates saasSkillDiscoveryResponse;
        TSearchResponse searchObject(TString{MEGAMIND_TUNNELLER_RESPONSE}, "", TRTLogger::NullLogger(),
                                     /* initDataSources= */ false);
        NiceMock<TMockResponses> responses{};
        EXPECT_CALL(responses, QueryTokensStatsResponse(_)).WillRepeatedly(ReturnRef(tokensStatsResponse));
        EXPECT_CALL(responses, SaasSkillDiscoveryResponse(_)).WillRepeatedly(ReturnRef(saasSkillDiscoveryResponse));
        EXPECT_CALL(responses, WebSearchResponse(_)).WillRepeatedly(ReturnRef(searchObject));

        const TString previousScenarioName = "previousScenarioName";
        NiceMock<TMockSession> session{};
        EXPECT_CALL(session, GetPreviousScenarioName()).WillRepeatedly(ReturnRef(previousScenarioName));
        EXPECT_CALL(session, GetScenarioSession(_))
            .WillRepeatedly(ReturnRef(TSessionProto::TScenarioSession::default_instance()));
        EXPECT_CALL(context, Sensors()).WillRepeatedly(ReturnRef(sensors));
        EXPECT_CALL(context, Responses()).WillRepeatedly(ReturnRef(responses));
        EXPECT_CALL(context, Session()).WillRepeatedly(Return(&session));
        EXPECT_CALL(context, StackEngineCore())
            .WillRepeatedly(ReturnRef(NMegamind::TStackEngineCore::default_instance()));

        /* InvalidRequest_ReturnNoWinnerScenario */ {
            auto speechKitRequest = TSpeechKitRequestBuilder{TStringBuf("{}")}.Build();

            EXPECT_CALL(responses, WizardResponse(_)).Times(0);

            auto event = TTextInputEvent{TEvent{}};
            EXPECT_CALL(context, ExpFlags()).WillRepeatedly(ReturnRef(speechKitRequest.ExpFlags()));
            EXPECT_CALL(context, Event()).WillRepeatedly(Return(&event));
            EXPECT_CALL(context, SpeechKitRequest()).WillRepeatedly(Return(speechKitRequest));

            TCommonScenarioWalker walker{globalCtx};
            const auto response = walker.RunPreClassifyStage(walkerCtx);

            UNIT_ASSERT(response.Scenarios.empty());
            UNIT_ASSERT(response.Error.Defined());
            UNIT_ASSERT(response.Error->ErrorMsg == "Failed to parse request event.");
        }
        /* SemanticFrameInput */ {
            const TString semanticFrameName{"personal_assistant.scenarios.search"};
            const auto event = TTextInputEvent{TEvent{}};

            EXPECT_CALL(context, Event()).WillRepeatedly(Return(&event));
            EXPECT_CALL(context, Logger()).WillRepeatedly(ReturnRef(TRTLogger::StderrLogger()));

            const TString scenarioName = "_scenario_";
            const auto disableExp = [&context](const auto& exp) {
                EXPECT_CALL(context, HasExpFlag(exp)).WillRepeatedly(Return(false));
            };
            disableExp(EXP_PREFIX_MM_DISABLE_PROTOCOL_SCENARIO + scenarioName);
            disableExp(EXP_DONT_DEFER_APPLY);
            IContext::TExpFlags expFlags;
            EXPECT_CALL(context, ExpFlags()).WillRepeatedly(ReturnRef(expFlags));

            const auto scenarioConfig = [&] {
                TScenarioConfig config{};
                config.SetName(scenarioName);
                config.SetEnabled(true);
                config.AddLanguages(ELang::L_RUS);
                config.AddAcceptedFrames(semanticFrameName);
                config.MutableHandlers()->SetRequestType(NAlice::ERequestType::AppHostPure);
                return config;
            }();
            const TScenarioInfraConfig scenarioInfraConfig{};
            scenarioConfigRegistry.AddScenarioConfig(scenarioConfig);
            EXPECT_CALL(context, ScenarioConfig(_)).WillRepeatedly(ReturnRef(scenarioInfraConfig));
            NMegamindAppHost::TLaunchedScenarios launchedScenarios;
            launchedScenarios.AddScenarios()->SetName(scenarioName);
            ahCtx.TestCtx().AddProtobufItem(launchedScenarios, NMegamind::AH_ITEM_LAUNCHED_SCENARIOS,
                                            NAppHost::EContextItemKind::Input);

            NMegamind::TMementoData mementoData{};
            EXPECT_CALL(context, MementoData()).WillRepeatedly(ReturnRef(mementoData));

            EXPECT_CALL(session, GetActions()).WillRepeatedly(Return(
                ::google::protobuf::Map<TString, NScenarios::TFrameAction>{[&](){
                    ::google::protobuf::Map<TString, NScenarios::TFrameAction> map;
                    NScenarios::TFrameAction frameAction;
                    frameAction.MutableNluHint()->SetFrameName("hint_frame_1");
                    frameAction.MutableFrame()->SetName(semanticFrameName);
                    map["some_action_1"] = frameAction;

                    frameAction.MutableNluHint()->SetFrameName("hint_frame_2");
                    frameAction.MutableParsedUtterance()->MutableFrame()->SetName(semanticFrameName);
                    map["some_action_2"] = frameAction;

                    frameAction.MutableNluHint()->SetFrameName("hint_frame_3");
                    frameAction.MutableParsedUtterance()
                        ->MutableTypedSemanticFrame()
                        ->MutableSearchSemanticFrame()
                        ->MutableQuery()
                        ->SetStringValue("some query");
                    map["some_action_3"] = frameAction;
                    return map;
                }()}
            ));
            const TString prevProductScenarioName = "prev product scenario name";
            EXPECT_CALL(session, GetPreviousProductScenarioName()).WillRepeatedly(ReturnRef(prevProductScenarioName));

            TVector<std::tuple<TWizardResponse, TString>> cases{
                /* TypedSemanticFrameEvent */
                {
                    TWizardResponse(),
                    TString{R"({
                        "header": {
                            "request_id": "d34df00d-c135-4227-8cf8-386d7d989237"
                        },
                        "request": {
                            "event": {
                                "name": "@@mm_semantic_frame",
                                "payload": {
                                    "typed_semantic_frame": {
                                        "search_semantic_frame": {
                                            "query": {
                                                "string_value": "что такое ананас"
                                            }
                                        }
                                    },
                                    "utterance": "что такое ананас",
                                    "analytics": {
                                        "product_scenario": "search",
                                        "origin": "Scenario",
                                        "purpose": "get_factoid"
                                    }
                                },
                                "type": "server_action"
                            }
                        }
                    })"}
                },
                /* RecognizedAction */
                {
                    CreateWizardResponseWithFrame("hint_frame_1", ACTION_RECOGNIZER_SOURCE_LABEL),
                    TString{R"({
                        "request": {
                            "event": {
                                "name":"",
                                "type":"text_input",
                                "text":"что такое ананас"
                            }
                        }
                    })"}
                },
                /* RecognizedAction */
                {
                    CreateWizardResponseWithFrame("hint_frame_2", ACTION_RECOGNIZER_SOURCE_LABEL),
                    TString{R"({
                        "request": {
                            "event": {
                                "name":"",
                                "type":"text_input",
                                "text":"что такое ананас"
                            }
                        }
                    })"}
                },
                /* RecognizedAction */
                {
                    CreateWizardResponseWithFrame("hint_frame_3", ACTION_RECOGNIZER_SOURCE_LABEL),
                    TString{R"({
                        "request": {
                            "event": {
                                "name":"",
                                "type":"text_input",
                                "text":"что такое ананас"
                            }
                        }
                    })"}
                }
            };

            TCommonScenarioWalker walker{globalCtx};
            size_t caseNumber = 0;
            for (const auto& [wizardResponse, textSkr] : cases) {
                EXPECT_CALL(responses, WizardResponse(_)).WillRepeatedly(ReturnRef(wizardResponse));
                auto speechKitRequest = TSpeechKitRequestBuilder{TStringBuf{textSkr}}.Build();
                EXPECT_CALL(context, ExpFlags()).WillRepeatedly(ReturnRef(speechKitRequest.ExpFlags()));
                EXPECT_CALL(context, SpeechKitRequest()).WillRepeatedly(Return(speechKitRequest));
                const auto response = walker.RunPostClassifyStage(walkerCtx);
                TMaybe<TError> scenarioError{};
                Cerr << "Errors): " << response.Errors.ErrorsToString() << Endl;

                response.Errors.ForEachError(
                    [&scenarioName, &scenarioError](const auto& scenario, const auto& stage, const auto& error) {
                        Cerr << "Scenario " << scenario << " error " << error << " on stage " << bool(stage == NImpl::STAGE_ASK) << Endl;
                        if (scenario == scenarioName && stage == NImpl::STAGE_ASK) {
                            scenarioError = error;
                        }
                    });
                UNIT_ASSERT_C(scenarioError.Defined(), "Case #" << caseNumber);
                // Now it's acceptable to ensure that mm tried to ask the scenario

                // Note: initially test contains string "no response (timeout)", but latest version may return
                // simple "no response" answer
                Cerr << "Scenario Error: " << scenarioError->ErrorMsg << Endl;
                UNIT_ASSERT_C(scenarioError->ErrorMsg.Contains("scenario__scenario__run_pure_response"), "Case #" << caseNumber);
                Cerr << "Scenario Error: " << scenarioError->ErrorMsg << Endl;
                ++caseNumber;
            }
        }
    }

    Y_UNIT_TEST(VersionInResponse) {
        StrictMock<TMockWalkerRequestContextTestWrapper> walkerRequestContextTestWrapper;
        TScenarioResponse response{/* scenarioName= */ "alice.builder", /* requestFrames= */ {},
            /* scenarioAcceptsAnyInput= */ true};
        TResponseBuilderProto proto;
        proto.SetScenarioName(response.GetScenarioName());
        proto.MutableResponse();

        auto skr = TSpeechKitRequestBuilder(TSpeechKitRequestBuilder::EPredefined::MinimalWithTextEvent).Build();
        auto request = CreateRequest(IEvent::CreateEvent(skr.Event()), skr);
        response.ForceBuilder(skr, request, NMegamind::TGuidGenerator{}, std::move(proto));
        TCommonScenarioWalker scenarioWalker(walkerRequestContextTestWrapper.Get().GlobalCtx());
        NMegamind::TMegamindAnalyticsInfoBuilder megamindAnalyticsInfoBuilder;
        NMegamind::TAnalyticsInfoBuilder analyticsInfoBuilder;
        NMegamind::TUserInfoBuilder userInfoBuilder;
        TState state;
        auto scenario = MakeSimpleShared<NiceMock<TMockScenario>>("123", false);

        auto scenarioWrapper = MakeIntrusive<NiceMock<TMockScenarioWrapper>>();
        EXPECT_CALL(*scenarioWrapper, GetScenario())
            .WillRepeatedly(Invoke([scenario=std::move(scenario)]()->const TScenario& { return *scenario;}));
        NMegamind::TModifiersStorage modifiersStorage;
        EXPECT_CALL(*scenarioWrapper, GetModifiersStorage())
            .WillRepeatedly(ReturnRef(modifiersStorage));
        EXPECT_CALL(*scenarioWrapper, GetApplyEnv(_, _))
            .WillRepeatedly(Return(TLightScenarioEnv(walkerRequestContextTestWrapper.Get().Ctx(), request, {}, state,
                                                     analyticsInfoBuilder, userInfoBuilder)));
        EXPECT_CALL(*scenarioWrapper, GetAnalyticsInfo())
            .WillRepeatedly(ReturnRef(analyticsInfoBuilder));
        TVector<TSemanticFrame> semanticFramesToMatchPostroll;
        EXPECT_CALL(*scenarioWrapper, GetSemanticFrames())
            .WillRepeatedly(ReturnRef(semanticFramesToMatchPostroll));

        TQualityStorage qualityStorage;
        NMegamind::TProactivityLogStorage proactivityLogStorage;
        TVector<TSemanticFrame> matchedSemanticFrames;

        const auto res = scenarioWalker.TryApplyAndFinalizeOrRenderError(
            /* disableApply= */ true, response, scenarioWrapper, request, walkerRequestContextTestWrapper.Get(),
            TRTLogger::StderrLogger(), ELanguage::LANG_RUS, megamindAnalyticsInfoBuilder, proactivityLogStorage,
            qualityStorage, /* proactivity= */ {}, TCommonScenarioWalker::ECalledFrom::RunStage,
            matchedSemanticFrames,
            /* postAnalyticsFiller= */ [](TScenarioWrapperPtr /* wrapper */) -> void {});

        UNIT_ASSERT(response.BuilderIfExists()->GetSKRProto().GetVersion() == NAlice::VERSION_STRING);
    }

    Y_UNIT_TEST(PassResponseFrameToProactivityWithMemento) {
        NJson::TJsonValue skrJson = NJson::ReadJsonFastTree(SKR_POSTROLL_MODIFIER_WITH_MEMENTO);
        NMegamind::NMementoApi::TRespGetAllObjects mementoObjects;
        JsonToProto(
            NJson::ReadJsonFastTree(PROACTIVITY_STORAGE_LAST_POSTROLL_VIEWS),
            *mementoObjects.MutableUserConfigs()->MutableProactivityConfig()->MutableStorage()
        );
        skrJson["request"]["raw_personal_data"] = "{}";
        auto skr = TSpeechKitRequestBuilder(skrJson).Build();
        auto request = CreateRequest(IEvent::CreateEvent(skr.Event()), skr);

        StrictMock<TMockWalkerRequestContextTestWrapper> walkerRequestContextTestWrapper(skr, std::move(mementoObjects));
        TCommonScenarioWalker scenarioWalker(walkerRequestContextTestWrapper.Get().GlobalCtx());

        TScenarioConfig scenarioConfig;
        scenarioConfig.SetName("scenario name");
        TConfigBasedAppHostPureProtocolScenario scenario(scenarioConfig);

        TScenarioInfraConfig scenarioInfraConfig;
        TMockContext applyContext;
        EXPECT_CALL(applyContext, ScenarioConfig(_)).WillRepeatedly(ReturnRef(scenarioInfraConfig));
        EXPECT_CALL(applyContext, Logger()).WillRepeatedly(ReturnRef(TRTLogger::NullLogger()));
        EXPECT_CALL(applyContext, Sensors()).WillRepeatedly(ReturnRef(walkerRequestContextTestWrapper.Get().GlobalCtx().ServiceSensors()));
        TClientInfo clientInfo{TClientInfoProto{}};
        EXPECT_CALL(applyContext, ClientInfo()).WillRepeatedly(ReturnRef(clientInfo));

        const IScenarioWrapper::TSemanticFrames requestFrames = {};
        auto scenarioWrapper = MakeIntrusive<TAppHostPureProtocolScenarioWrapper>(
            scenario, applyContext, requestFrames, NMegamind::TGuidGenerator{},
            EDeferredApplyMode::DontDeferApply, /* restoreAllFromSession= */ true,
            walkerRequestContextTestWrapper.Get().ItemProxyAdapter());

        NMegamind::TMegamindAnalyticsInfoBuilder megamindAnalyticsInfoBuilder;
        TQualityStorage qualityStorage;
        NMegamind::TProactivityLogStorage proactivityLogStorage;
        TVector<TSemanticFrame> matchedSemanticFrames;

        {
            TScenarioResponse response{/* scenarioName= */ "alice.builder", /* requestFrames= */ {}, /* scenarioAcceptsAnyInput= */ true};
            TResponseBuilderProto proto;
            proto.SetScenarioName(response.GetScenarioName());
            proto.MutableResponse();
            response.ForceBuilder(skr, request, NMegamind::TGuidGenerator{}, std::move(proto));

            const auto res = scenarioWalker.TryApplyAndFinalizeOrRenderError(
                /* disableApply= */ true, response, scenarioWrapper, request, walkerRequestContextTestWrapper.Get(),
                TRTLogger::NullLogger(), ELanguage::LANG_RUS, megamindAnalyticsInfoBuilder,
                proactivityLogStorage, qualityStorage, /* proactivity= */ {},
                TCommonScenarioWalker::ECalledFrom::RunStage, matchedSemanticFrames,
                /* postAnalyticsFiller= */ [](TScenarioWrapperPtr /* wrapper */) -> void {});

            UNIT_ASSERT(response.BuilderIfExists());
            UNIT_ASSERT_MESSAGES_EQUAL(TSemanticFrame{}, response.GetResponseSemanticFrame());

            const auto proactivityInfo = megamindAnalyticsInfoBuilder.BuildProto().GetModifiersInfo().GetProactivity();
            UNIT_ASSERT(!proactivityInfo.GetAppended());
            UNIT_ASSERT(proactivityInfo.GetPostrollClickIds().empty());
            UNIT_ASSERT(proactivityInfo.GetPostrollClicks().empty());
        }
        {
            TScenarioResponse response{/* scenarioName= */ "alice.builder", /* requestFrames= */ {}, /* scenarioAcceptsAnyInput= */ true};
            TResponseBuilderProto proto;
            proto.SetScenarioName(response.GetScenarioName());
            proto.MutableResponse();
            TSemanticFrame frame;
            frame.SetName("test_response_frame_name");
            *proto.MutableSemanticFrame() = frame;
            response.ForceBuilder(skr, request, NMegamind::TGuidGenerator{}, std::move(proto));

            const auto res = scenarioWalker.TryApplyAndFinalizeOrRenderError(
                /* disableApply= */ true, response, scenarioWrapper, request, walkerRequestContextTestWrapper.Get(),
                TRTLogger::NullLogger(), ELanguage::LANG_RUS, megamindAnalyticsInfoBuilder,
                proactivityLogStorage, qualityStorage, /* proactivity= */ {},
                TCommonScenarioWalker::ECalledFrom::RunStage, matchedSemanticFrames,
                /* postAnalyticsFiller= */ [](TScenarioWrapperPtr /* wrapper */) -> void {});

            UNIT_ASSERT(response.BuilderIfExists());
            UNIT_ASSERT_MESSAGES_EQUAL(frame, response.GetResponseSemanticFrame());

            const auto proactivityInfo = megamindAnalyticsInfoBuilder.BuildProto().GetModifiersInfo().GetProactivity();
            UNIT_ASSERT(!proactivityInfo.GetAppended());
            UNIT_ASSERT(proactivityInfo.GetPostrollClickIds().size() == 1);
            UNIT_ASSERT(*proactivityInfo.GetPostrollClickIds().begin() == "postroll_id");
            UNIT_ASSERT(proactivityInfo.GetPostrollClicks().size() == 1);
            UNIT_ASSERT(proactivityInfo.GetPostrollClicks().begin()->GetItemId() == "postroll_id");
            UNIT_ASSERT(proactivityInfo.GetPostrollClicks().begin()->GetBaseId() == "base_id");
            UNIT_ASSERT(proactivityInfo.GetPostrollClicks().begin()->GetItemInfo() == "item_info");
            UNIT_ASSERT(proactivityInfo.GetPostrollClicks().begin()->GetShowReqId() == "postroll-show-req-id");
        }
    }

    Y_UNIT_TEST(PassBegemotFramesToProactivityWithMemento) {
        NJson::TJsonValue skrJson = NJson::ReadJsonFastTree(SKR_POSTROLL_MODIFIER_WITH_MEMENTO);
        NMegamind::NMementoApi::TRespGetAllObjects mementoObjects;
        JsonToProto(
            NJson::ReadJsonFastTree(PROACTIVITY_STORAGE_LAST_POSTROLL_VIEWS),
            *mementoObjects.MutableUserConfigs()->MutableProactivityConfig()->MutableStorage()
        );
        skrJson["request"]["raw_personal_data"] = "{}";
        auto skr = TSpeechKitRequestBuilder(skrJson).Build();
        auto request = CreateRequest(IEvent::CreateEvent(skr.Event()), skr);

        StrictMock<TMockWalkerRequestContextTestWrapper> walkerRequestContextTestWrapper(skr, std::move(mementoObjects));
        TCommonScenarioWalker scenarioWalker(walkerRequestContextTestWrapper.Get().GlobalCtx());

        TScenarioConfig scenarioConfig;
        scenarioConfig.SetName("scenario name");
        TConfigBasedAppHostPureProtocolScenario scenario(scenarioConfig);

        TScenarioInfraConfig scenarioInfraConfig;
        TMockContext applyContext;
        EXPECT_CALL(applyContext, ScenarioConfig(_)).WillRepeatedly(ReturnRef(scenarioInfraConfig));
        EXPECT_CALL(applyContext, Logger()).WillRepeatedly(ReturnRef(TRTLogger::NullLogger()));
        EXPECT_CALL(applyContext, Sensors()).WillRepeatedly(ReturnRef(walkerRequestContextTestWrapper.Get().GlobalCtx().ServiceSensors()));
        TClientInfo clientInfo{TClientInfoProto{}};
        EXPECT_CALL(applyContext, ClientInfo()).WillRepeatedly(ReturnRef(clientInfo));

        {
            TScenarioResponse response{/* scenarioName= */ "alice.builder", /* requestFrames= */ {}, /* scenarioAcceptsAnyInput= */ true};
            TResponseBuilderProto proto;
            proto.SetScenarioName(response.GetScenarioName());
            proto.MutableResponse();
            TSemanticFrame frame;
            frame.SetName("test_response_frame_name");
            *proto.MutableSemanticFrame() = frame;
            response.ForceBuilder(skr, request, NMegamind::TGuidGenerator{}, std::move(proto));

            TSemanticFrame requestFrame;
            requestFrame.SetName("request_frame_not_match");
            TVector<TSemanticFrame> matchedSemanticFrames = {requestFrame};
            auto wrapper = MakeIntrusive<TAppHostPureProtocolScenarioWrapper>(
                scenario, applyContext, matchedSemanticFrames, NMegamind::TGuidGenerator{},
                EDeferredApplyMode::DeferredCall, /* restoreAllFromSession= */ true,
                walkerRequestContextTestWrapper.Get().ItemProxyAdapter());

            NMegamind::TMegamindAnalyticsInfoBuilder megamindAnalyticsInfoBuilder;
            TQualityStorage qualityStorage;
            NMegamind::TProactivityLogStorage proactivityLogStorage;

            const auto res = scenarioWalker.TryApplyAndFinalizeOrRenderError(
                /* disableApply= */ true, response, wrapper, request, walkerRequestContextTestWrapper.Get(),
                TRTLogger::NullLogger(), ELanguage::LANG_RUS, megamindAnalyticsInfoBuilder,
                proactivityLogStorage, qualityStorage, /* proactivity= */ {},
                TCommonScenarioWalker::ECalledFrom::RunStage, matchedSemanticFrames,
                /* postAnalyticsFiller= */ [](TScenarioWrapperPtr /* wrapper */) -> void {});

            const auto proactivityInfo = megamindAnalyticsInfoBuilder.BuildProto().GetModifiersInfo().GetProactivity();
            UNIT_ASSERT(!proactivityInfo.GetAppended());
            UNIT_ASSERT(proactivityInfo.GetPostrollClickIds().empty());
            UNIT_ASSERT(proactivityInfo.GetPostrollClicks().empty());
        }
        {
            TScenarioResponse response{/* scenarioName= */ "alice.builder", /* requestFrames= */ {}, /* scenarioAcceptsAnyInput= */ true};
            TResponseBuilderProto proto;
            proto.SetScenarioName(response.GetScenarioName());
            proto.MutableResponse();
            response.ForceBuilder(skr, request, NMegamind::TGuidGenerator{}, std::move(proto));

            TSemanticFrame requestFrame;
            requestFrame.SetName("test_response_frame_name");
            TVector<TSemanticFrame> matchedSemanticFrames = {requestFrame};
            auto wrapper = MakeIntrusive<TAppHostPureProtocolScenarioWrapper>(
                scenario, applyContext, matchedSemanticFrames, NMegamind::TGuidGenerator{},
                EDeferredApplyMode::DeferredCall, /* restoreAllFromSession= */ true,
                walkerRequestContextTestWrapper.Get().ItemProxyAdapter());

            NMegamind::TMegamindAnalyticsInfoBuilder megamindAnalyticsInfoBuilder;
            TQualityStorage qualityStorage;
            NMegamind::TProactivityLogStorage proactivityLogStorage;

            const auto res = scenarioWalker.TryApplyAndFinalizeOrRenderError(
                /* disableApply= */ true, response, wrapper, request, walkerRequestContextTestWrapper.Get(),
                TRTLogger::NullLogger(), ELanguage::LANG_RUS, megamindAnalyticsInfoBuilder,
                proactivityLogStorage, qualityStorage, /* proactivity= */ {},
                TCommonScenarioWalker::ECalledFrom::RunStage, matchedSemanticFrames,
                /* postAnalyticsFiller= */ [](TScenarioWrapperPtr /* wrapper */) -> void {});

            const auto proactivityInfo = megamindAnalyticsInfoBuilder.BuildProto().GetModifiersInfo().GetProactivity();
            UNIT_ASSERT(!proactivityInfo.GetAppended());
            UNIT_ASSERT(proactivityInfo.GetPostrollClickIds().size() == 1);
            UNIT_ASSERT(*proactivityInfo.GetPostrollClickIds().begin() == "postroll_id");
            UNIT_ASSERT(proactivityInfo.GetPostrollClicks().size() == 1);
            UNIT_ASSERT(proactivityInfo.GetPostrollClicks().begin()->GetItemId() == "postroll_id");
            UNIT_ASSERT(proactivityInfo.GetPostrollClicks().begin()->GetBaseId() == "base_id");
            UNIT_ASSERT(proactivityInfo.GetPostrollClicks().begin()->GetItemInfo() == "item_info");
            UNIT_ASSERT(proactivityInfo.GetPostrollClicks().begin()->GetShowReqId() == "postroll-show-req-id");
        }
    }

    Y_UNIT_TEST(TestGetMatchedSemanticFrames) {
        StrictMock<TMockWalkerRequestContextTestWrapper> walkerRequestContextTestWrapper;
        TCommonScenarioWalker scenarioWalker(walkerRequestContextTestWrapper.Get().GlobalCtx());

        TScenarioConfig scenarioConfig;
        scenarioConfig.SetName("scenario name");
        TConfigBasedAppHostPureProtocolScenario scenario(scenarioConfig);

        TScenarioInfraConfig scenarioInfraConfig;
        TMockContext applyContext;
        EXPECT_CALL(applyContext, ScenarioConfig(_)).WillRepeatedly(ReturnRef(scenarioInfraConfig));
        EXPECT_CALL(applyContext, Logger()).WillRepeatedly(ReturnRef(TRTLogger::NullLogger()));
        EXPECT_CALL(applyContext, Sensors()).WillRepeatedly(ReturnRef(walkerRequestContextTestWrapper.Get().GlobalCtx().ServiceSensors()));
        TClientInfo clientInfo{TClientInfoProto{}};
        EXPECT_CALL(applyContext, ClientInfo()).WillRepeatedly(ReturnRef(clientInfo));

        TSemanticFrame wrapperFrame;
        wrapperFrame.SetName("test_wrapper_frame_name");
        TVector<TSemanticFrame> wrapperSemanticFrames = {wrapperFrame};

        const auto wrapperWithFrames = MakeIntrusive<TAppHostPureProtocolScenarioWrapper>(
            scenario, applyContext, wrapperSemanticFrames, NMegamind::TGuidGenerator{},
            EDeferredApplyMode::DeferredCall, /* restoreAllFromSession= */ true,
            walkerRequestContextTestWrapper.Get().ItemProxyAdapter());

        const auto wrapperEmptyFrames = MakeIntrusive<TAppHostPureProtocolScenarioWrapper>(
            scenario, applyContext, /* wrapperSemanticFrames */ TVector<TSemanticFrame>{}, NMegamind::TGuidGenerator{},
            EDeferredApplyMode::DeferredCall, /* restoreAllFromSession= */ true,
            walkerRequestContextTestWrapper.Get().ItemProxyAdapter());

        NScenarios::TInput input;
        TSemanticFrame& sessionFrame = *input.AddSemanticFrames();
        sessionFrame.SetName("test_session_frame_name");
        TState state;

        const auto session = MakeSessionBuilder()
            ->SetPreviousScenarioName("previous_scenario_name")
            .SetScenarioSession("previous_scenario_name", NewScenarioSession(state))
            .SetInput(input)
            .Build();

        auto matchedSemanticFrames = scenarioWalker.GetMatchedSemanticFrames(session.Get(), wrapperWithFrames, TCommonScenarioWalker::ECalledFrom::RunStage);
        UNIT_ASSERT(matchedSemanticFrames.size() == 1);
        UNIT_ASSERT(matchedSemanticFrames[0].GetName() == "test_wrapper_frame_name");

        matchedSemanticFrames = scenarioWalker.GetMatchedSemanticFrames(session.Get(), wrapperEmptyFrames, TCommonScenarioWalker::ECalledFrom::RunStage);
        UNIT_ASSERT(matchedSemanticFrames.empty());

        matchedSemanticFrames = scenarioWalker.GetMatchedSemanticFrames(session.Get(), wrapperWithFrames, TCommonScenarioWalker::ECalledFrom::ApplyStage);
        UNIT_ASSERT(matchedSemanticFrames.size() == 1);
        UNIT_ASSERT(matchedSemanticFrames[0].GetName() == "test_session_frame_name");

        matchedSemanticFrames = scenarioWalker.GetMatchedSemanticFrames(nullptr, wrapperWithFrames, TCommonScenarioWalker::ECalledFrom::ApplyStage);
        UNIT_ASSERT(matchedSemanticFrames.empty() == 1);
    }

    Y_UNIT_TEST(TryApplyAndFinalizeOrRenderErrorHttpCodeNotSet) {
        StrictMock<TMockWalkerRequestContextTestWrapper> walkerRequestContextTestWrapper;
        TScenarioResponse response{/* scenarioName= */ "alice.builder", /* requestFrames= */ {},
            /* scenarioAcceptsAnyInput= */ true};
        TResponseBuilderProto proto;
        proto.SetScenarioName(response.GetScenarioName());
        proto.MutableResponse();

        auto skr = TSpeechKitRequestBuilder(TSpeechKitRequestBuilder::EPredefined::MinimalWithTextEvent).Build();
        auto request = CreateRequest(IEvent::CreateEvent(skr.Event()), skr);
        response.ForceBuilder(skr, request, NMegamind::TGuidGenerator{}, std::move(proto));
        TCommonScenarioWalker scenarioWalker(walkerRequestContextTestWrapper.Get().GlobalCtx());
        NMegamind::TMegamindAnalyticsInfoBuilder megamindAnalyticsInfoBuilder;
        NMegamind::TAnalyticsInfoBuilder analyticsInfoBuilder;
        NMegamind::TUserInfoBuilder userInfoBuilder;
        TState state;
        auto scenario = MakeSimpleShared<NiceMock<TMockScenario>>("123", false);

        auto scenarioWrapper = MakeIntrusive<NiceMock<TMockScenarioWrapper>>();
        EXPECT_CALL(*scenarioWrapper, GetScenario())
            .WillRepeatedly(Invoke([scenario=std::move(scenario)]()->const TScenario& { return *scenario;}));
        NMegamind::TModifiersStorage modifiersStorage;
        EXPECT_CALL(*scenarioWrapper, GetModifiersStorage())
            .WillRepeatedly(ReturnRef(modifiersStorage));
        EXPECT_CALL(*scenarioWrapper, GetApplyEnv(_, _))
            .WillRepeatedly(Return(TLightScenarioEnv(walkerRequestContextTestWrapper.Get().Ctx(), request, {}, state,
                                                     analyticsInfoBuilder, userInfoBuilder)));
        EXPECT_CALL(*scenarioWrapper, GetAnalyticsInfo())
            .WillRepeatedly(ReturnRef(analyticsInfoBuilder));
        TVector<TSemanticFrame> semanticFramesToMatchPostroll;
        EXPECT_CALL(*scenarioWrapper, GetSemanticFrames())
            .WillRepeatedly(ReturnRef(semanticFramesToMatchPostroll));

        TQualityStorage qualityStorage;
        NMegamind::TProactivityLogStorage proactivityLogStorage;
        TVector<TSemanticFrame> matchedSemanticFrames;

        response.SetHttpCode(HTTP_UNASSIGNED_512);

        const auto res = scenarioWalker.TryApplyAndFinalizeOrRenderError(
            /* disableApply= */ true, response, scenarioWrapper, request, walkerRequestContextTestWrapper.Get(),
            TRTLogger::NullLogger(), ELanguage::LANG_RUS, megamindAnalyticsInfoBuilder, proactivityLogStorage,
            qualityStorage, /* proactivity= */ {},
            TCommonScenarioWalker::ECalledFrom::RunStage, matchedSemanticFrames,
            /* postAnalyticsFiller= */ [](TScenarioWrapperPtr /* wrapper */) -> void {});

        UNIT_ASSERT(res.IsSuccess());
        UNIT_ASSERT(response.GetHttpCode() == HTTP_OK);
    }

    Y_UNIT_TEST(GetScenariosFromFlag) {
        UNIT_ASSERT_EQUAL((THashSet<TString>{NImpl::ALL_SCENARIOS}), NImpl::GetScenariosFromFlag({}));
        UNIT_ASSERT_EQUAL((THashSet<TString>{"a", "b"}), NImpl::GetScenariosFromFlag("a;b"));
        UNIT_ASSERT_EQUAL((THashSet<TString>{"a"}), NImpl::GetScenariosFromFlag("a"));
    }

    Y_UNIT_TEST(SkipScenariosWithEmptyRequiredDataSource) {
        NiceMock<TMockWalkerRequestContextTestWrapper> walkerRequestContextTestWrapper;
        NiceMock<TMockRunWalkerRequestCtx> runWalkerRequestContext;
        NiceMock<TMockContext> context;

        auto requestCtx = NMegamind::TMockRequestCtx::CreateStrict(walkerRequestContextTestWrapper.Get().GlobalCtx());
        EXPECT_CALL(runWalkerRequestContext, Ctx()).WillRepeatedly(ReturnRef(context));
        EXPECT_CALL(runWalkerRequestContext, RequestCtx()).WillRepeatedly(ReturnRef(requestCtx));
        auto& logger = TRTLogger::StderrLogger();
        EXPECT_CALL(context, Logger()).WillRepeatedly(ReturnRef(logger));
        TMockSensors sensors;
        EXPECT_CALL(context, Sensors()).WillRepeatedly(ReturnRef(sensors));
        TMockResponses responses;
        EXPECT_CALL(context, Responses()).WillRepeatedly(ReturnRef(responses));
        NMegamind::TMementoData mementoData{};
        EXPECT_CALL(context, MementoData()).WillRepeatedly(ReturnRef(mementoData));

        IContext::TExpFlags expFlags;
        EXPECT_CALL(context, ExpFlags()).WillRepeatedly(ReturnRef(expFlags));

        auto speechKitRequest = TSpeechKitRequestBuilder(TSpeechKitRequestBuilder::EPredefined::MinimalWithTextEvent)
            .Build();
        EXPECT_CALL(context, SpeechKitRequest()).WillRepeatedly(Return(speechKitRequest));

        TClientInfo clientInfo(TClientInfoProto{});
        EXPECT_CALL(context, ClientInfo()).WillRepeatedly(ReturnRef(clientInfo));

        const TString SCENARIO_A_NAME = "scenarioA";
        const TString SCENARIO_B_NAME = "scenarioB";

        TScenarioRegistry registry;
        TScenarioInfraConfig infraConfig;

        for (const auto& scenarioName : {SCENARIO_A_NAME, SCENARIO_B_NAME}) {
            TScenarioConfig config;
            config.SetName(scenarioName);
            config.MutableHandlers()->SetBaseUrl(scenarioName);
            config.AddLanguages(::NAlice::ELang::L_RUS);
            auto* dataSourceParam = config.AddDataSources();
            dataSourceParam->SetType(EDataSourceType::VINS_WIZARD_RULES);
            dataSourceParam->SetIsRequired(scenarioName == SCENARIO_A_NAME);

            EXPECT_CALL(context, ScenarioConfig(scenarioName)).WillRepeatedly(ReturnRef(infraConfig));

            auto scenarioHolder = MakeHolder<TMockProtocolScenario>(config);
            if (scenarioName != SCENARIO_A_NAME) {
                const auto& scenario = *scenarioHolder.Get();
                EXPECT_CALL(scenario, StartRun(_, _, _)).WillOnce(Return(Success()));
                EXPECT_CALL(scenario, FinishRun(_, _)).WillOnce(InvokeWithoutArgs(
                    []() -> NScenarios::TScenarioRunResponse {
                        NScenarios::TScenarioRunResponse response;
                        response.MutableResponseBody()->MutableLayout()->AddCards()->SetText("Ok");
                        return response;
                    }
                ));
            }
            registry.RegisterConfigBasedAppHostPureProtocolScenario(std::move(scenarioHolder));
        }


        TScenarioToRequestFrames scenarioToRequestFrames;
        for (const auto& ref : registry.GetScenarioRefs()) {
            scenarioToRequestFrames[ref] = {};
        }
        const auto request = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);

        NMegamind::TGuidGenerator guidGenerator;
        auto selectedWrappers = NImpl::MakeScenarioWrappers(scenarioToRequestFrames, context,
                                                            guidGenerator, EDeferredApplyMode::DeferApply,
                                                            walkerRequestContextTestWrapper.Get().ItemProxyAdapter());

        NMegamind::TMockDataSources dataSources;
        NScenarios::TDataSource dataSource;

        EXPECT_CALL(dataSources, GetDataSource(EDataSourceType::VINS_WIZARD_RULES))
            .WillRepeatedly(ReturnRef(dataSource));
        TWalkerResponse response{};

        TFakeStorage storage{};
        TFormulasDescription formulasDescription{};
        TFormulasStorage formulasStorage{storage, formulasDescription};

        TFactorStorage factorStorage = CreateStorage();

        TCommonScenarioWalker scenarioWalker(walkerRequestContextTestWrapper.Get().GlobalCtx());
        auto ahCtx = NAlice::NMegamind::NTesting::TTestAppHostCtx{walkerRequestContextTestWrapper.Get().GlobalCtx()};
        scenarioWalker.InitScenarios(selectedWrappers, request, context, factorStorage, formulasStorage, dataSources,
                                     ILightWalkerRequestCtx::ERunStage::Prepare, response, ahCtx.ItemProxyAdapter());
        UNIT_ASSERT_C(response.GetScenarioErrors().Empty(), response.GetScenarioErrors().ErrorsToString());
    }

    Y_UNIT_TEST(DoNotSkipScenarioWithEmptyRequiredDataSourceOnCallback) {
        NiceMock<TMockWalkerRequestContextTestWrapper> walkerRequestContextTestWrapper;
        NiceMock<TMockRunWalkerRequestCtx> runWalkerRequestContext;
        NiceMock<TMockContext> context;
        auto requestCtx = NMegamind::TMockRequestCtx::CreateStrict(walkerRequestContextTestWrapper.Get().GlobalCtx());
        EXPECT_CALL(runWalkerRequestContext, Ctx()).WillRepeatedly(ReturnRef(context));
        EXPECT_CALL(runWalkerRequestContext, RequestCtx()).WillRepeatedly(ReturnRef(requestCtx));
        auto& logger = TRTLogger::StderrLogger();
        EXPECT_CALL(context, Logger()).WillRepeatedly(ReturnRef(logger));
        TMockSensors sensors;
        EXPECT_CALL(context, Sensors()).WillRepeatedly(ReturnRef(sensors));
        TMockResponses responses;
        EXPECT_CALL(context, Responses()).WillRepeatedly(ReturnRef(responses));
        NMegamind::TMementoData mementoData{};
        EXPECT_CALL(context, MementoData()).WillRepeatedly(ReturnRef(mementoData));

        IContext::TExpFlags expFlags;
        EXPECT_CALL(context, ExpFlags()).WillRepeatedly(ReturnRef(expFlags));

        const TString SCENARIO_NAME = "scenarioA";

        auto speechKitRequest = TSpeechKitRequestBuilder(TSpeechKitRequestBuilder::EPredefined::MinimalWithoutEvent)
            .SetProtoPatcher([&SCENARIO_NAME](NMegamind::TSpeechKitInitContext& initCtx){
                // Prepare callback to scenarioA
                auto& event = *initCtx.EventProtoPtr;
                event.SetType(EEventType::server_action);
                auto* payload = event.MutablePayload();
                google::protobuf::Value value;
                value.set_string_value(TString{SCENARIO_NAME});
                payload->mutable_fields()->insert({"@scenario_name", value});
            })
            .Build();
        EXPECT_CALL(context, SpeechKitRequest()).WillRepeatedly(Return(speechKitRequest));

        TClientInfo clientInfo(TClientInfoProto{});
        EXPECT_CALL(context, ClientInfo()).WillRepeatedly(ReturnRef(clientInfo));

        TScenarioRegistry registry;
        TScenarioInfraConfig infraConfig;

        TScenarioConfig config;
        config.SetName(SCENARIO_NAME);
        config.MutableHandlers()->SetBaseUrl(SCENARIO_NAME);
        config.AddLanguages(::NAlice::ELang::L_RUS);
        auto* dataSourceParam = config.AddDataSources();
        dataSourceParam->SetType(EDataSourceType::VINS_WIZARD_RULES);
        dataSourceParam->SetIsRequired(true);

        EXPECT_CALL(context, ScenarioConfig(SCENARIO_NAME)).WillRepeatedly(ReturnRef(infraConfig));

        auto scenarioHolder = MakeHolder<StrictMock<TMockProtocolScenario>>(config);

        const auto& scenario = *scenarioHolder.Get();
        // We expect start / finish being called for this scenario
        EXPECT_CALL(scenario, StartRun(_, _, _)).WillOnce(Return(Success()));
        EXPECT_CALL(scenario, FinishRun(_, _)).WillOnce(InvokeWithoutArgs(
            []() -> NScenarios::TScenarioRunResponse {
                NScenarios::TScenarioRunResponse response;
                response.MutableResponseBody()->MutableLayout()->AddCards()->SetText("Ok");
                return response;
            }
        ));

        EXPECT_CALL(scenario, GetAcceptedFrames).WillRepeatedly(Return(TVector<TString>{}));

        registry.RegisterConfigBasedAppHostPureProtocolScenario(std::move(scenarioHolder));

        TScenarioToRequestFrames scenarioToRequestFrames;
        for (const auto& ref : registry.GetScenarioRefs()) {
            scenarioToRequestFrames[ref] = {};
        }
        const auto request = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);

        NMegamind::TGuidGenerator guidGenerator;
        auto selectedWrappers = NImpl::MakeScenarioWrappers(scenarioToRequestFrames, context,
                                                            guidGenerator, EDeferredApplyMode::DeferApply,
                                                            walkerRequestContextTestWrapper.Get().ItemProxyAdapter());

        NMegamind::TMockDataSources dataSources;
        NScenarios::TDataSource dataSource;

        EXPECT_CALL(dataSources, GetDataSource(EDataSourceType::VINS_WIZARD_RULES))
            .WillRepeatedly(ReturnRef(dataSource));
        TWalkerResponse response{};

        TFakeStorage storage{};
        TFormulasDescription formulasDescription{};
        TFormulasStorage formulasStorage{storage, formulasDescription};

        TFactorStorage factorStorage = CreateStorage();

        TCommonScenarioWalker scenarioWalker(walkerRequestContextTestWrapper.Get().GlobalCtx());
        auto ahCtx = NAlice::NMegamind::NTesting::TTestAppHostCtx{walkerRequestContextTestWrapper.Get().GlobalCtx()};
        scenarioWalker.InitScenarios(selectedWrappers, request, context, factorStorage, formulasStorage, dataSources,
                                     ILightWalkerRequestCtx::ERunStage::Prepare, response, ahCtx.ItemProxyAdapter());
        UNIT_ASSERT_C(response.GetScenarioErrors().Empty(), response.GetScenarioErrors().ErrorsToString());
    }

    Y_UNIT_TEST(AsyncScenarioAsk) {
        NiceMock<TMockWalkerRequestContextTestWrapper> walkerRequestContextTestWrapper;
        NiceMock<TMockRunWalkerRequestCtx> runWalkerRequestContext;
        NiceMock<TMockContext> context;
        auto requestCtx = NMegamind::TMockRequestCtx::CreateStrict(walkerRequestContextTestWrapper.Get().GlobalCtx());
        EXPECT_CALL(runWalkerRequestContext, Ctx()).WillRepeatedly(ReturnRef(context));
        EXPECT_CALL(runWalkerRequestContext, RequestCtx()).WillRepeatedly(ReturnRef(requestCtx));
        auto& logger = TRTLogger::StderrLogger();
        EXPECT_CALL(context, Logger()).WillRepeatedly(ReturnRef(logger));
        TMockSensors sensors;
        EXPECT_CALL(context, Sensors()).WillRepeatedly(ReturnRef(sensors));
        TMockResponses responses;
        EXPECT_CALL(context, Responses()).WillRepeatedly(ReturnRef(responses));

        EXPECT_CALL(context, HasExpFlag(_)).WillRepeatedly(Return(false));
        IContext::TExpFlags expFlags;
        EXPECT_CALL(context, ExpFlags()).WillRepeatedly(ReturnRef(expFlags));

        TFakeThreadPool scenarioThreads;
        EXPECT_CALL(context, RequestThreads()).WillRepeatedly(ReturnRef(scenarioThreads));

        auto speechKitRequest = TSpeechKitRequestBuilder(TSpeechKitRequestBuilder::EPredefined::MinimalWithTextEvent)
            .Build();
        EXPECT_CALL(context, SpeechKitRequest()).WillRepeatedly(Return(speechKitRequest));

        TClientInfo clientInfo(TClientInfoProto{});
        EXPECT_CALL(context, ClientInfo()).WillRepeatedly(ReturnRef(clientInfo));

        const TString SCENARIO_A_NAME = "scenarioA";
        const TString SCENARIO_B_NAME = "scenarioB";

        TScenarioRegistry registry;
        TScenarioInfraConfig infraConfig;
        TScenarioWrapperPtrs selectedWrappers;

        std::once_flag flags[2];

        TVector<TSemanticFrame> semanticFrames;

        size_t cnt = 0;
        for (const auto& scenarioName : {SCENARIO_A_NAME, SCENARIO_B_NAME}) {
            TScenarioConfig config;
            config.SetName(scenarioName);
            config.MutableHandlers()->SetBaseUrl(scenarioName);

            EXPECT_CALL(context, ScenarioConfig(scenarioName)).WillRepeatedly(ReturnRef(infraConfig));

            auto scenarioHolder = MakeHolder<TMockProtocolScenario>(config);
            const auto& scenario = *scenarioHolder.Get();
            registry.RegisterConfigBasedAppHostPureProtocolScenario(std::move(scenarioHolder));

            auto wrapper = MakeIntrusive<NiceMock<TMockScenarioWrapper>>();
            EXPECT_CALL(*wrapper, Accept(_))
                .Times(AtLeast(1))
                .WillRepeatedly(Invoke([&scenario](const IScenarioVisitor& visitor) { visitor.Visit(scenario); }));
            EXPECT_CALL(*wrapper, GetScenario()).Times(AtLeast(1)).WillRepeatedly(ReturnRef(scenario));
            EXPECT_CALL(*wrapper, GetAskFlag()).WillOnce(ReturnRef(flags[cnt]));
            EXPECT_CALL(*wrapper, IsSuccess()).WillOnce(Return(true));

            EXPECT_CALL(*wrapper, GetSemanticFrames()).WillOnce(ReturnRef(semanticFrames));

            selectedWrappers.push_back(wrapper);

            ++cnt;
        }

        TScenarioToRequestFrames scenarioToRequestFrames;
        for (const auto& ref : registry.GetScenarioRefs()) {
            scenarioToRequestFrames[ref] = {};
        }
        const auto request = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);

        NMegamind::TMockDataSources dataSources;
        TWalkerResponse response{};

        NiceMock<NTestingHelpers::TMockRequestEventListener> eventListener;

        TFakeStorage storage{};
        TFormulasDescription formulasDescription{};
        TFormulasStorage formulasStorage{storage, formulasDescription};

        TFactorStorage factorStorage = CreateStorage();

        TCommonScenarioWalker scenarioWalker(walkerRequestContextTestWrapper.Get().GlobalCtx());
        auto ahCtx = NAlice::NMegamind::NTesting::TTestAppHostCtx{walkerRequestContextTestWrapper.Get().GlobalCtx()};
        scenarioWalker.InitScenarios(selectedWrappers, request, context, factorStorage, formulasStorage, dataSources,
                                     ILightWalkerRequestCtx::ERunStage::Prepare, response, ahCtx.ItemProxyAdapter());
        UNIT_ASSERT_C(response.GetScenarioErrors().Empty(), response.GetScenarioErrors().ErrorsToString());
    }

    Y_UNIT_TEST(TestSendUdpMetrics) {
        TVector<TScenarioResponse> scenarioResponses;
        StrictMock<TMockSensors> sensors;

        auto addResponse = [&](const TString& scenarioName, const TString& intent, bool isWinner, bool isIrrelevant) {
            auto& response = scenarioResponses.emplace_back(TScenarioResponse(/* scenarioName= */ scenarioName, /* requestFrames= */ {},
                                                            /* scenarioAcceptsAnyInput= */ true));
            NAlice::TFeatures features;
            features.MutableScenarioFeatures()->SetIntent(intent);
            features.MutableScenarioFeatures()->SetIsIrrelevant(isIrrelevant);
            response.SetFeatures(features);
            EXPECT_CALL(sensors, IncRate(NMonitoring::TLabels{
                {NSignal::UDP_SENSOR, NSignal::IS_WINNER_SCENARIO},
                {NSignal::INTENT, intent},
                {NSignal::SCENARIO_NAME, scenarioName},
                {NSignal::IS_WINNER_SCENARIO, ToString(false)}
            }));
            if (isWinner) {
                EXPECT_CALL(sensors, IncRate(NMonitoring::TLabels{
                    {NSignal::UDP_SENSOR, NSignal::IS_WINNER_SCENARIO},
                    {NSignal::INTENT, intent},
                    {NSignal::SCENARIO_NAME, scenarioName},
                    {NSignal::IS_WINNER_SCENARIO, ToString(true)}
                }));
            }
            if (isIrrelevant) {
                EXPECT_CALL(sensors, IncRate(NMonitoring::TLabels{
                    {NSignal::UDP_SENSOR, NSignal::IS_IRRELEVANT},
                    {NSignal::INTENT, intent},
                    {NSignal::SCENARIO_NAME, scenarioName},
                }));
            }
        };

        addResponse("first", "intent_first", /* isWinner= */ true, /* isIrrelevant= */ false);
        addResponse("second", "intent_second", /* isWinner= */ false, /* isIrrelevant= */ false);
        addResponse("third", "intent_third", /* isWinner= */ false, /* isIrrelevant= */ true);

        TMockGlobalContextTestWrapper globalCtxWrapper;
        TCommonScenarioWalker walker{globalCtxWrapper.Get()};
        walker.SendUdpSensors(scenarioResponses, sensors, /* isWinnerScenario= */ false);

        walker.SendUdpSensors(scenarioResponses, sensors, /* isWinnerScenario= */ true);

    }

    Y_UNIT_TEST(TestGetResponseFromCombinator) {
        NiceMock<TMockWalkerRequestContextTestWrapper> walkerRequestContextTestWrapper;

        NiceMock<TMockResponses> responses{};
        TSearchResponse searchObject("", "", TRTLogger::NullLogger(), /* initDataSources= */ false);
        EXPECT_CALL(responses, WebSearchResponse(_)).WillRepeatedly(ReturnRef(searchObject));
        EXPECT_CALL(walkerRequestContextTestWrapper.GetCtx(), Responses()).WillRepeatedly(ReturnRef(responses));


        NiceMock<TMockRunWalkerRequestCtx> runWalkerRequestContext;
        NMegamind::NTesting::TMockModifierRequestFactory modifierRequestFactory;
        EXPECT_CALL(runWalkerRequestContext, ModifierRequestFactory).WillRepeatedly(ReturnRef(modifierRequestFactory));
        auto factorStorage = NMegamind::CreateFactorStorage(NMegamind::CreateFactorDomain());
        TState scenarioState;
        const TString scenarioName = "COMBINATOR";

        NMegamind::TMegamindAnalyticsInfo mmAnalytics;
        mmAnalytics.SetParentProductScenarioName("prev_psn");

        auto prevScenarioSession = NewScenarioSession(scenarioState);
        const auto& session = MakeSessionBuilder()
            ->SetPreviousScenarioName(scenarioName)
            .SetScenarioSession(scenarioName, prevScenarioSession)
            .SetMegamindAnalyticsInfo(mmAnalytics)
            .Build();

        EXPECT_CALL(walkerRequestContextTestWrapper.GetCtx(), Session()).WillRepeatedly(Return(session.Get()));

        EXPECT_CALL(runWalkerRequestContext, Ctx()).WillRepeatedly(ReturnRef(walkerRequestContextTestWrapper.GetCtx()));
        EXPECT_CALL(runWalkerRequestContext, RequestCtx()).WillRepeatedly(ReturnRef(walkerRequestContextTestWrapper.Get().RequestCtx()));
        EXPECT_CALL(runWalkerRequestContext, FactorStorage()).WillRepeatedly(ReturnRef(factorStorage));

        NMegamind::TCombinatorConfigProto combinatorConfigProto;
        combinatorConfigProto.SetName("COMBINATOR");
        TCombinatorConfig combinatorConfig{combinatorConfigProto};
        TCommonScenarioWalker scenarioWalker{walkerRequestContextTestWrapper.Get().GlobalCtx()};
        auto skr = TSpeechKitRequestBuilder(TSpeechKitRequestBuilder::EPredefined::MinimalWithTextEvent).Build();
        auto request = CreateRequest(IEvent::CreateEvent(skr.Event()), skr);
        IScenarioWalker::TRequestState requestState{std::move(request)};
        IScenarioWalker::TRunState runState;

        IScenarioWalker::TPreClassifyState preClassifyState;

        const TString name = "scenario1";
        NScenarios::TScenarioResponseBody body;
        body.MutableAnalyticsInfo()->SetProductScenarioName("product_scenario_name" + name);
        body.MutableSemanticFrame()->SetName("scenario_semantic_frame");
        (*body.MutableFrameActions())["action"].MutableFrame()->SetName("action_frame");
        TScenarioResponse resp{/* scenarioName= */ name, /* requestFrames= */ {},
                               /* scenarioAcceptsAnyInput= */ true};
        resp.SetResponseBody(body);
        runState.Response.AddScenarioResponse(std::move(resp));

        TScenarioConfig config;
        config.SetName(name);
        auto scenario = MakeSimpleShared<TConfigBasedAppHostPureProtocolScenario>(config);
        auto wrapper = MakeIntrusive<NiceMock<TMockScenarioWrapper>>();
        EXPECT_CALL(*wrapper, GetScenario()).WillRepeatedly(Invoke([scenario]()->const TScenario& { return *scenario; }));
        NMegamind::TAnalyticsInfoBuilder analyticsBuilder;
        EXPECT_CALL(*wrapper, GetAnalyticsInfo()).WillRepeatedly(ReturnRef(analyticsBuilder));
        NMegamind::TUserInfoBuilder userInfoBuilder;
        EXPECT_CALL(*wrapper, GetUserInfo()).WillRepeatedly(ReturnRef(userInfoBuilder));
        TState state;
        EXPECT_CALL(*wrapper, GetApplyEnv(_, _))
            .WillRepeatedly(Return(TLightScenarioEnv(walkerRequestContextTestWrapper.Get().Ctx(), requestState.Request, {}, state,
                                                     analyticsBuilder, userInfoBuilder)));

        TScenarioWrapperPtrs wrappers{
            wrapper
        };
        preClassifyState.ScenarioWrappers = std::move(wrappers);

        /* testIrrelevant */ {
            auto ahCtx = NAlice::NMegamind::NTesting::TTestAppHostCtx{walkerRequestContextTestWrapper.Get().GlobalCtx()};
            NScenarios::TCombinatorResponse combinatorResponse;
            combinatorResponse.MutableCombinatorsAnalyticsInfo()->SetCombinatorProductName("combinator_product_name");
            combinatorResponse.MutableResponse()->MutableFeatures()->SetIsIrrelevant(true);
            ahCtx.TestCtx().AddProtobufItem(combinatorResponse, TString{NMegamind::AH_ITEM_COMBINATOR_RESPONSE_PREFIX} + scenarioName, NAppHost::EContextItemKind::Input);
            NMegamind::TItemProxyAdapter& itemProxyAdapter = ahCtx.ItemProxyAdapter();
            EXPECT_CALL(runWalkerRequestContext, ItemProxyAdapter()).WillRepeatedly(ReturnRef(itemProxyAdapter));

            auto responseProto = scenarioWalker.TryChooseCombinator(runWalkerRequestContext, combinatorConfig, NMegamindAppHost::TCombinatorProto::Run);
            UNIT_ASSERT(!responseProto.Defined());
        }

        /* testCorrectCombinatorAnswer */ {
            StrictMock<TMockSensors> sensors;
            EXPECT_CALL(sensors, IncRate(NMonitoring::TLabels{
                {"combinator_name", scenarioName},
                {"name", "combinator_wins_per_second"}
            }));
            EXPECT_CALL(sensors, IncRate(NMonitoring::TLabels{
                {"combinator_name", scenarioName},
                {"scenario_name", "scenario1"},
                {"name", "scenario_in_combinator_wins_per_second"}
            }));
            EXPECT_CALL(walkerRequestContextTestWrapper.GetCtx(), Sensors()).WillRepeatedly(ReturnRef(sensors));
            auto ahCtx = NAlice::NMegamind::NTesting::TTestAppHostCtx{walkerRequestContextTestWrapper.Get().GlobalCtx()};
            NScenarios::TCombinatorResponse combinatorResponseProto;
            combinatorResponseProto.MutableCombinatorsAnalyticsInfo()->SetCombinatorProductName("combinator_product_name");
            *combinatorResponseProto.AddUsedScenarios() = "scenario1";
            ahCtx.TestCtx().AddProtobufItem(combinatorResponseProto, TString{NMegamind::AH_ITEM_COMBINATOR_RESPONSE_PREFIX} + scenarioName, NAppHost::EContextItemKind::Input);
            NMegamind::TItemProxyAdapter& itemProxyAdapter = ahCtx.ItemProxyAdapter();
            EXPECT_CALL(runWalkerRequestContext, ItemProxyAdapter()).WillRepeatedly(ReturnRef(itemProxyAdapter));

            const auto actualProto = scenarioWalker.TryChooseCombinator(runWalkerRequestContext, combinatorConfig, NMegamindAppHost::TCombinatorProto::Run);
            UNIT_ASSERT(actualProto.Defined());
            UNIT_ASSERT_MESSAGES_EQUAL(*actualProto, combinatorResponseProto);
            NMegamind::TCombinatorResponse combinatorResponse{combinatorConfig};
            combinatorResponse.SetResponseProto(*actualProto);
            const auto result = scenarioWalker.RenderCombinatorResponse(
                runWalkerRequestContext, runState.Response, std::move(runState.AnalyticsInfoBuilder), requestState.Request,
                preClassifyState, combinatorResponse);

            UNIT_ASSERT(!result.Defined());
            UNIT_ASSERT(runState.Response.Scenarios.size() == 2);
            UNIT_ASSERT(runState.Response.Scenarios[0].GetScenarioName() == scenarioName);

            auto analyticsInfoExpected = JsonToProto<NMegamind::TMegamindAnalyticsInfo>(JsonFromString(TStringBuf(R"({
                "analytics_info": {
                    "scenario1": {
                        "scenario_analytics_info": {
                            "product_scenario_name": "product_scenario_namescenario1"
                        },
                        "semantic_frame": {
                            "name": "scenario_semantic_frame"
                        },
                        "frame_actions": {
                            "action": {
                                "frame": {
                                    "name": "action_frame"
                                }
                            }
                        },
                        "parent_product_scenario_name": "prev_psn"
                    }
                },
                "combinators_analytics_info": {
                    "combinator_product_name": "combinator_product_name"
                }
            })")));
            (*analyticsInfoExpected.MutableAnalyticsInfo())["scenario1"].SetVersion(NAlice::VERSION_STRING);
            UNIT_ASSERT_MESSAGES_EQUAL(
                runState.Response.AnalyticsInfoBuilder.BuildProto(),
                analyticsInfoExpected
            );
        }
    }

    Y_UNIT_TEST(FinalizeWrapper) {
        StrictMock<TMockWalkerRequestContextTestWrapper> walkerRequestContextTestWrapper;
        TScenarioResponse response{/* scenarioName= */ TEST_SCENARIO_NAME, /* requestFrames= */ {},
                                   /* scenarioAcceptsAnyInput= */ true};

        auto skr = TSpeechKitRequestBuilder(TSpeechKitRequestBuilder::EPredefined::MinimalWithTextEvent).Build();
        auto request = CreateRequest(IEvent::CreateEvent(skr.Event()), skr);

        TCommonScenarioWalker scenarioWalker(walkerRequestContextTestWrapper.Get().GlobalCtx());
        NMegamind::TMegamindAnalyticsInfoBuilder megamindAnalyticsInfoBuilder;
        NMegamind::TAnalyticsInfoBuilder analyticsInfoBuilder;
        TState state;

        TMockContext ctx;
        TScenarioInfraConfig scenarioInfraConfig;
        TClientInfo clientInfo{TClientInfoProto{}};
        {
            EXPECT_CALL(ctx, ScenarioConfig(_)).WillRepeatedly(ReturnRef(scenarioInfraConfig));
            EXPECT_CALL(ctx, Logger()).WillRepeatedly(ReturnRef(TRTLogger::NullLogger()));
            EXPECT_CALL(ctx, Sensors())
                .WillRepeatedly(ReturnRef(walkerRequestContextTestWrapper.Get().GlobalCtx().ServiceSensors()));
            EXPECT_CALL(ctx, ClientInfo()).WillRepeatedly(ReturnRef(clientInfo));
        }

        TScenarioConfig scenarioConfig;
        scenarioConfig.SetName(TEST_SCENARIO_NAME);
        StrictMock<TMockProtocolScenario> scenario{scenarioConfig};

        const IScenarioWrapper::TSemanticFrames requestFrames = {};

        NMegamind::TGuidGenerator guidGenerator{};
        auto wrapper = MakeIntrusive<TAppHostPureProtocolScenarioWrapper>(
            scenario, ctx, requestFrames, guidGenerator,
            EDeferredApplyMode::DeferApply, /* restoreAllFromSession= */ true,
            walkerRequestContextTestWrapper.Get().ItemProxyAdapter());

        NScenarios::TScenarioRunResponse scenarioResponse;
        {
            auto* layout = scenarioResponse.MutableResponseBody()->MutableLayout();
            constexpr auto CARD_TEXT = "There is no spoon";
            layout->SetOutputSpeech(CARD_TEXT);
            layout->AddCards()->SetText(CARD_TEXT);
            EXPECT_CALL(scenario, FinishRun(_, _)).WillOnce(Return(scenarioResponse));
        }

        const auto error = wrapper->Ask(request, ctx, response);
        UNIT_ASSERT_C(!error.Defined(), *error);

        UNIT_ASSERT(response.ResponseBodyIfExists());
        UNIT_ASSERT_VALUES_EQUAL(JsonFromProto(scenarioResponse.GetResponseBody()),
                                 JsonFromProto(*response.ResponseBodyIfExists()));

        TQualityStorage qualityStorage;
        NMegamind::TProactivityLogStorage proactivityLogStorage;
        TVector<TSemanticFrame> matchedSemanticFrames;

        const auto res = scenarioWalker.TryApplyAndFinalizeOrRenderError(
            /* disableApply= */ true, response, wrapper, request, walkerRequestContextTestWrapper.Get(),
            TRTLogger::NullLogger(), ELanguage::LANG_RUS, megamindAnalyticsInfoBuilder, proactivityLogStorage,
            qualityStorage, /* proactivity= */ {}, TCommonScenarioWalker::ECalledFrom::RunStage,
            matchedSemanticFrames,
            /* postAnalyticsFiller= */ [](TScenarioWrapperPtr /* wrapper */) -> void {});
        UNIT_ASSERT_C(response.BuilderIfExists(), "TryApplyAndFinalizeOrRenderError should answer with builder");
        UNIT_ASSERT_VALUES_EQUAL(SpeechKitResponseToJson(response.BuilderIfExists()->GetSKRProto())["response"],
                                 JsonFromString(R"({
            "cards": [
                {
                    "type": "simple_text",
                    "text": "There is no spoon"
                }
            ],
            "card": {
                "type": "simple_text",
                "text": "There is no spoon"
            },
            "experiments": {},
            "directives": [],
            "directives_execution_policy": "BeforeSpeech",
            "templates": {}
        })"));
    }

    Y_UNIT_TEST(AskModifiersFailure) {
        StrictMock<TMockWalkerRequestContextTestWrapper> walkerRequestContextTestWrapper;
        TScenarioResponse response{/* scenarioName= */ TEST_SCENARIO_NAME, /* requestFrames= */ {},
                                   /* scenarioAcceptsAnyInput= */ true};

        auto skr = TSpeechKitRequestBuilder(TSpeechKitRequestBuilder::EPredefined::MinimalWithTextEvent).Build();
        auto request = CreateRequest(IEvent::CreateEvent(skr.Event()), skr);

        auto& ctx = walkerRequestContextTestWrapper.GetCtx();

        TScenarioInfraConfig scenarioInfraConfig;
        EXPECT_CALL(ctx, ScenarioConfig(_)).WillRepeatedly(ReturnRef(scenarioInfraConfig));
        EXPECT_CALL(ctx, Logger()).WillRepeatedly(ReturnRef(TRTLogger::NullLogger()));
        EXPECT_CALL(ctx, Sensors())
            .WillRepeatedly(ReturnRef(walkerRequestContextTestWrapper.Get().GlobalCtx().ServiceSensors()));

        StrictMock<TMockProtocolScenario> scenario{[] {
                                                       TScenarioConfig scenarioConfig;
                                                       scenarioConfig.SetName(TEST_SCENARIO_NAME);
                                                       return scenarioConfig;
                                                   }()
        };

        const IScenarioWrapper::TSemanticFrames requestFrames = {};
        NMegamind::TGuidGenerator guidGenerator{};

        auto wrapper = MakeIntrusive<TAppHostPureProtocolScenarioWrapper>(
            scenario, ctx, requestFrames, guidGenerator, EDeferredApplyMode::DeferApply,
            /* restoreAllFromSession= */ true, walkerRequestContextTestWrapper.Get().ItemProxyAdapter());

        NScenarios::TScenarioRunResponse scenarioResponse;
        {
            auto* layout = scenarioResponse.MutableResponseBody()->MutableLayout();
            constexpr auto CARD_TEXT = "Some text i want to replace but fail to";
            layout->SetOutputSpeech(CARD_TEXT);
            layout->AddCards()->SetText(CARD_TEXT);
            EXPECT_CALL(scenario, FinishRun(_, _)).WillOnce(Return(scenarioResponse));
        }

        const auto error = wrapper->Ask(request, ctx, response);
        UNIT_ASSERT_C(!error.Defined(), *error);

        UNIT_ASSERT(response.ResponseBodyIfExists());
        UNIT_ASSERT_VALUES_EQUAL(JsonFromProto(scenarioResponse.GetResponseBody()),
                                 JsonFromProto(*response.ResponseBodyIfExists()));

        TQualityStorage qualityStorage;
        NMegamind::TProactivityLogStorage proactivityLogStorage;
        TVector<TSemanticFrame> matchedSemanticFrames;

        StrictMock<NMegamind::NTesting::TMockModifierRequestFactory> modifierRequestFactory;
        EXPECT_CALL(walkerRequestContextTestWrapper.Get(), ModifierRequestFactory)
            .WillRepeatedly(ReturnRef(modifierRequestFactory));
        EXPECT_CALL(modifierRequestFactory, ApplyModifierResponse).WillRepeatedly(Return(TError{TError::EType::NotFound}));

        TCommonScenarioWalker scenarioWalker(walkerRequestContextTestWrapper.Get().GlobalCtx());
        NMegamind::TMegamindAnalyticsInfoBuilder megamindAnalyticsInfoBuilder;
        const auto res = scenarioWalker.TryApplyAndFinalizeOrRenderError(
            /* disableApply= */ true, response, wrapper, request, walkerRequestContextTestWrapper.Get(),
            TRTLogger::NullLogger(), ELanguage::LANG_RUS, megamindAnalyticsInfoBuilder, proactivityLogStorage,
            qualityStorage, /* proactivity= */ {}, TCommonScenarioWalker::ECalledFrom::RunStage,
            matchedSemanticFrames,
            /* postAnalyticsFiller= */ [](TScenarioWrapperPtr /* wrapper */) -> void {});

        UNIT_ASSERT_C(response.BuilderIfExists(), "TryApplyAndFinalizeOrRenderError should answer with builder");
        UNIT_ASSERT_VALUES_EQUAL(SpeechKitResponseToJson(response.BuilderIfExists()->GetSKRProto())["response"],
                                 JsonFromString(R"({
            "cards": [
                {
                    "type": "simple_text",
                    "text": "Прошу прощения, что-то сломалось. Спросите попозже, пожалуйста.",
                }
            ],
            "card": {
                "type": "simple_text",
                "text": "Прошу прощения, что-то сломалось. Спросите попозже, пожалуйста.",
            },
            "experiments": {},
            "directives": [],
            "meta": [
                {
                    "error_type": "not_found",
                    "type": "error",
                    "message": "_scenario_: "
                }
            ],
        })"));
    }

    Y_UNIT_TEST(AskModifiersSuccessWalkerRun) {
        StrictMock<TMockWalkerRequestContextTestWrapper> walkerRequestContextTestWrapper;
        TScenarioResponse response{/* scenarioName= */ TEST_SCENARIO_NAME, /* requestFrames= */ {},
                                   /* scenarioAcceptsAnyInput= */ true};

        auto skr = TSpeechKitRequestBuilder(TSpeechKitRequestBuilder::EPredefined::MinimalWithTextEvent).Build();
        auto request = CreateRequest(IEvent::CreateEvent(skr.Event()), skr);



        auto& ctx = walkerRequestContextTestWrapper.GetCtx();
        TScenarioInfraConfig scenarioInfraConfig;

        EXPECT_CALL(ctx, ScenarioConfig(_)).WillRepeatedly(ReturnRef(scenarioInfraConfig));
        EXPECT_CALL(ctx, Logger()).WillRepeatedly(ReturnRef(TRTLogger::NullLogger()));
        EXPECT_CALL(ctx, Sensors())
            .WillRepeatedly(ReturnRef(walkerRequestContextTestWrapper.Get().GlobalCtx().ServiceSensors()));

        StrictMock<TMockProtocolScenario> scenario{[] {
                                                       TScenarioConfig scenarioConfig;
                                                       scenarioConfig.SetName(TEST_SCENARIO_NAME);
                                                       return scenarioConfig;
                                                   }(),
        };

        const IScenarioWrapper::TSemanticFrames requestFrames = {};
        NMegamind::TGuidGenerator guidGenerator{};

        auto wrapper = MakeIntrusive<TAppHostPureProtocolScenarioWrapper>(
            scenario, ctx, requestFrames, guidGenerator, EDeferredApplyMode::DeferApply,
            /* restoreAllFromSession= */ true, walkerRequestContextTestWrapper.Get().ItemProxyAdapter());

        NScenarios::TScenarioRunResponse scenarioResponse;
        {
            auto* layout = scenarioResponse.MutableResponseBody()->MutableLayout();
            constexpr auto CARD_TEXT = "Some text i want to replace";
            layout->SetOutputSpeech(CARD_TEXT);
            layout->AddCards()->SetText(CARD_TEXT);
            EXPECT_CALL(scenario, FinishRun(_, _)).WillOnce(Return(scenarioResponse));
        }

        const auto error = wrapper->Ask(request, ctx, response);
        UNIT_ASSERT_C(!error.Defined(), *error);

        UNIT_ASSERT(response.ResponseBodyIfExists());
        UNIT_ASSERT_VALUES_EQUAL(JsonFromProto(scenarioResponse.GetResponseBody()),
                                 JsonFromProto(*response.ResponseBodyIfExists()));

        TQualityStorage qualityStorage;
        NMegamind::TProactivityLogStorage proactivityLogStorage;
        TVector<TSemanticFrame> matchedSemanticFrames;

        StrictMock<NMegamind::NTesting::TMockModifierRequestFactory> modifierRequestFactory;
        EXPECT_CALL(walkerRequestContextTestWrapper.Get(), ModifierRequestFactory)
            .WillRepeatedly(ReturnRef(modifierRequestFactory));
        EXPECT_CALL(modifierRequestFactory, ApplyModifierResponse)
            .WillRepeatedly(Invoke([](TScenarioResponse& scenarioResponse, NMegamind::TMegamindAnalyticsInfoBuilder&) -> TStatus
        {
            constexpr auto CARD_TEXT = "There is no spoon";
            auto* layout = scenarioResponse.ResponseBodyIfExists()->MutableLayout();
            *layout = {};
            layout->SetOutputSpeech(CARD_TEXT);
            layout->AddCards()->SetText(CARD_TEXT);
            return Success();
        }));

        TCommonScenarioWalker scenarioWalker(walkerRequestContextTestWrapper.Get().GlobalCtx());
        NMegamind::TMegamindAnalyticsInfoBuilder megamindAnalyticsInfoBuilder;
        NMegamind::TAnalyticsInfoBuilder analyticsInfoBuilder;

        const auto res = scenarioWalker.TryApplyAndFinalizeOrRenderError(
            /* disableApply= */ true, response, wrapper, request, walkerRequestContextTestWrapper.Get(),
            TRTLogger::NullLogger(), ELanguage::LANG_RUS, megamindAnalyticsInfoBuilder, proactivityLogStorage,
            qualityStorage, /* proactivity= */ {}, TCommonScenarioWalker::ECalledFrom::RunStage,
            matchedSemanticFrames,
            /* postAnalyticsFiller= */ [](TScenarioWrapperPtr /* wrapper */) -> void {});

        UNIT_ASSERT_C(response.BuilderIfExists(), "TryApplyAndFinalizeOrRenderError should answer with builder");
        UNIT_ASSERT_VALUES_EQUAL(SpeechKitResponseToJson(response.BuilderIfExists()->GetSKRProto())["response"],
                                 JsonFromString(R"({
            "cards": [
                {
                    "type": "simple_text",
                    "text": "There is no spoon"
                }
            ],
            "card": {
                "type": "simple_text",
                "text": "There is no spoon"
            },
            "experiments": {},
            "directives": [],
            "directives_execution_policy": "BeforeSpeech",
            "templates": {}
        })"));
    }

    Y_UNIT_TEST(RunFinalizeSuccess) {
        StrictMock<TMockWalkerRequestContextTestWrapper> walkerRequestContextTestWrapper;
        NiceMock<TMockResponses> responses{};
        TSearchResponse searchObject(TString{MEGAMIND_TUNNELLER_RESPONSE}, "", TRTLogger::NullLogger(),
                                     /* initDataSources= */ false);
        EXPECT_CALL(responses, WebSearchResponse(_)).WillRepeatedly(ReturnRef(searchObject));
        EXPECT_CALL(walkerRequestContextTestWrapper.GetCtx(), Responses()).WillRepeatedly(ReturnRef(responses));

        StrictMock<TMockRunWalkerRequestCtx> runCtx;
        NMegamind::NTesting::TMockModifierRequestFactory modifierRequestFactory;
        EXPECT_CALL(runCtx, ModifierRequestFactory).WillRepeatedly(ReturnRef(modifierRequestFactory));

        EXPECT_CALL(runCtx, PostClassifyState())
            .WillRepeatedly(ReturnRef(walkerRequestContextTestWrapper.GetPostClassifyState()));
        EXPECT_CALL(runCtx, Ctx()).WillRepeatedly(ReturnRef(walkerRequestContextTestWrapper.GetCtx()));

        auto skr = TSpeechKitRequestBuilder(TSpeechKitRequestBuilder::EPredefined::MinimalWithTextEvent).Build();
        auto request = CreateRequest(IEvent::CreateEvent(skr.Event()), skr);

        TCommonScenarioWalker scenarioWalker(walkerRequestContextTestWrapper.Get().GlobalCtx());

        TScenarioConfig scenarioConfig;
        scenarioConfig.SetName(TEST_SCENARIO_NAME);
        StrictMock<TMockProtocolScenario> scenario{scenarioConfig};

        TSemanticFrame frame;
        *frame.MutableName() = "frame_name";
        const IScenarioWrapper::TSemanticFrames requestFrames = {frame};

        NMegamind::TGuidGenerator guidGenerator{};
        auto wrapper = MakeIntrusive<TAppHostPureProtocolScenarioWrapper>(
            scenario, walkerRequestContextTestWrapper.GetCtx(), requestFrames, guidGenerator,
            EDeferredApplyMode::DeferApply,
            /* restoreAllFromSession= */ true,
            walkerRequestContextTestWrapper.Get().ItemProxyAdapter());

        IScenarioWalker::TPreClassifyState preClassifyState;
        preClassifyState.ScenarioWrappers = {wrapper};

        EXPECT_CALL(walkerRequestContextTestWrapper.GetPostClassifyState(), GetQualityStorage())
            .WillOnce(Return(TQualityStorage::default_instance()));

        NMegamind::TMegamindAnalyticsInfo analyticsInfo;
        *analyticsInfo.MutableOriginalUtterance() = "kek";

        EXPECT_CALL(walkerRequestContextTestWrapper.GetPostClassifyState(), GetAnalytics())
            .WillOnce(Return(analyticsInfo));

        EXPECT_CALL(walkerRequestContextTestWrapper.GetPostClassifyState(), GetWinnerScenario())
            .WillOnce(Return(TEST_SCENARIO_NAME));

        NMegamindAppHost::TScenarioErrorsProto scenarioErrors;
        {
            NMegamindAppHost::TScenarioErrorsProto_TScenarioError error;
            error.SetScenario("scenario2");
            error.SetStage("postclassify_stage");
            *error.MutableError() = NMegamind::ErrorToProto(TError{TError::EType::Http});
            *scenarioErrors.AddScenarioErrors() = std::move(error);
        }
        EXPECT_CALL(walkerRequestContextTestWrapper.GetPostClassifyState(), GetScenarioErrors())
            .WillOnce(Return(scenarioErrors));

        auto ahCtx = NAlice::NMegamind::NTesting::TTestAppHostCtx{walkerRequestContextTestWrapper.Get().GlobalCtx()};
        ahCtx.TestCtx().AddProtobufItem(NScenarios::TInput{}, InputItemName(TEST_SCENARIO_NAME),
                                        NAppHost::EContextItemKind::Input);
        EXPECT_CALL(runCtx, ItemProxyAdapter()).WillRepeatedly(ReturnRef(ahCtx.ItemProxyAdapter()));

        NScenarios::TScenarioRunResponse scenarioResponse;
        {
            auto* layout = scenarioResponse.MutableResponseBody()->MutableLayout();
            constexpr auto CARD_TEXT = "There is no spoon";
            layout->SetOutputSpeech(CARD_TEXT);
            layout->AddCards()->SetText(CARD_TEXT);

            auto* analytics = scenarioResponse.MutableResponseBody()->MutableAnalyticsInfo();
            analytics->SetProductScenarioName(TEST_PRODUCT_SCENARIO_NAME);
            EXPECT_CALL(scenario, FinishRun(_, _)).WillOnce(Return(scenarioResponse));
        }
        NMegamind::TMegamindAnalyticsInfoBuilder dummyAnalytics;
        dummyAnalytics.SetWinnerScenarioName("not_winner_scenario");
        auto walkerResponse = scenarioWalker.RunFinalize(runCtx, preClassifyState, request, std::move(dummyAnalytics));

        UNIT_ASSERT(!walkerResponse.Scenarios.empty());
        const auto& response = walkerResponse.Scenarios.front();

        UNIT_ASSERT_C(response.BuilderIfExists(), "RunFinalize should answer with builder");

        UNIT_ASSERT_VALUES_EQUAL(SpeechKitResponseToJson(response.BuilderIfExists()->GetSKRProto())["response"],
                                 JsonFromString(R"({
            "cards": [
                {
                    "type": "simple_text",
                    "text": "There is no spoon"
                }
            ],
            "card": {
                "type": "simple_text",
                "text": "There is no spoon"
            },
            "meta": [
                {
                  "error_type": "http",
                  "type": "error",
                  "message": "{\"scenario_name\":\"scenario2\",\"message\":\"\",\"stage\":\"postclassify_stage\"}"
                }
            ],
            "experiments": {},
            "directives": [],
            "directives_execution_policy": "BeforeSpeech",
            "templates": {}
        })"));

        auto analyticsInfoExpected = JsonToProto<NMegamind::TMegamindAnalyticsInfo>(JsonFromString(TStringBuf(R"({
            "analytics_info": {
                "_scenario_": {
                    "scenario_analytics_info": {
                        "product_scenario_name": "_product_scenario_"
                    },
                    "matched_semantic_frames": {
                        "name": "frame_name"
                    },
                }
            },
            "original_utterance": "kek",
            "winner_scenario": {
              "name": "_scenario_"
            }
        })")));
        (*analyticsInfoExpected.MutableAnalyticsInfo())[TEST_SCENARIO_NAME].SetVersion(NAlice::VERSION_STRING);
        auto analyticsInfoActual = walkerResponse.AnalyticsInfoBuilder.BuildProto();
        UNIT_ASSERT_VALUES_EQUAL(analyticsInfoExpected.GetAnalyticsInfo().size(),
                                 analyticsInfoActual.GetAnalyticsInfo().size());
        UNIT_ASSERT_MESSAGES_EQUAL(analyticsInfoExpected.GetAnalyticsInfo().at(TEST_SCENARIO_NAME),
                                   analyticsInfoActual.GetAnalyticsInfo().at(TEST_SCENARIO_NAME));
        UNIT_ASSERT_VALUES_EQUAL(analyticsInfoExpected.GetOriginalUtterance(),
                                 analyticsInfoActual.GetOriginalUtterance());
        UNIT_ASSERT_MESSAGES_EQUAL(analyticsInfoExpected.GetWinnerScenario(), analyticsInfoActual.GetWinnerScenario());
    }

    Y_UNIT_TEST(RunFinalizeNoWinner) {
        StrictMock<TMockWalkerRequestContextTestWrapper> walkerRequestContextTestWrapper;
        StrictMock<TMockRunWalkerRequestCtx> runCtx;
        EXPECT_CALL(runCtx, PostClassifyState())
            .WillRepeatedly(ReturnRef(walkerRequestContextTestWrapper.GetPostClassifyState()));
        EXPECT_CALL(runCtx, Ctx()).WillRepeatedly(ReturnRef(walkerRequestContextTestWrapper.GetCtx()));
        EXPECT_CALL(runCtx, RequestCtx())
            .WillRepeatedly(ReturnRef(walkerRequestContextTestWrapper.Get().RequestCtx()));
        EXPECT_CALL(runCtx, Rng())
            .WillRepeatedly(ReturnRef(walkerRequestContextTestWrapper.GetGlobalCtxWrapper().GetRng()));

        EXPECT_CALL(walkerRequestContextTestWrapper.GetCtx(), Language()).WillRepeatedly(Return(ELanguage::LANG_RUS));

        TMockResponses responses;
        TSearchResponse searchObject(TString{MEGAMIND_TUNNELLER_RESPONSE}, "", TRTLogger::NullLogger(),
                                     /* initDataSources= */ false);
        EXPECT_CALL(responses, WebSearchResponse(_)).WillRepeatedly(ReturnRef(searchObject));
        EXPECT_CALL(walkerRequestContextTestWrapper.GetCtx(), Responses()).WillRepeatedly(ReturnRef(responses));

        auto skr = TSpeechKitRequestBuilder(TSpeechKitRequestBuilder::EPredefined::MinimalWithTextEvent).Build();
        auto request = CreateRequest(IEvent::CreateEvent(skr.Event()), skr);

        TCommonScenarioWalker scenarioWalker(walkerRequestContextTestWrapper.Get().GlobalCtx());

        TScenarioConfig scenarioConfig;
        scenarioConfig.SetName(TEST_SCENARIO_NAME);
        StrictMock<TMockProtocolScenario> scenario{scenarioConfig};

        TSemanticFrame frame;
        *frame.MutableName() = "frame_name";
        const IScenarioWrapper::TSemanticFrames requestFrames = {frame};

        auto ahCtx = NAlice::NMegamind::NTesting::TTestAppHostCtx{walkerRequestContextTestWrapper.Get().GlobalCtx()};
        EXPECT_CALL(runCtx, ItemProxyAdapter()).WillRepeatedly(ReturnRef(ahCtx.ItemProxyAdapter()));
        NMegamind::TGuidGenerator guidGenerator{};
        auto wrapper = MakeIntrusive<TAppHostPureProtocolScenarioWrapper>(
            scenario, walkerRequestContextTestWrapper.GetCtx(), requestFrames, guidGenerator,
            EDeferredApplyMode::DeferApply,
            /* restoreAllFromSession= */ true,
            ahCtx.ItemProxyAdapter());

        IScenarioWalker::TPreClassifyState preClassifyState;
        preClassifyState.ScenarioWrappers = {wrapper};

        EXPECT_CALL(walkerRequestContextTestWrapper.GetPostClassifyState(), GetQualityStorage())
            .WillOnce(Return(TQualityStorage::default_instance()));

        NMegamind::TMegamindAnalyticsInfo analyticsInfo;
        *analyticsInfo.MutableOriginalUtterance() = "kek";

        EXPECT_CALL(walkerRequestContextTestWrapper.GetPostClassifyState(), GetAnalytics())
            .WillOnce(Return(analyticsInfo));

        EXPECT_CALL(walkerRequestContextTestWrapper.GetPostClassifyState(), GetWinnerScenario())
            .WillOnce(Return(TEST_SCENARIO_NAME));

        NMegamindAppHost::TScenarioErrorsProto scenarioErrors;
        {
            NMegamindAppHost::TScenarioErrorsProto_TScenarioError error;
            error.SetScenario("scenario2");
            error.SetStage("postclassify_stage");
            *error.MutableError() = NMegamind::ErrorToProto(TError{TError::EType::Http});
            *scenarioErrors.AddScenarioErrors() = std::move(error);
        }
        EXPECT_CALL(walkerRequestContextTestWrapper.GetPostClassifyState(), GetScenarioErrors())
            .WillOnce(Return(scenarioErrors));

        EXPECT_CALL(walkerRequestContextTestWrapper.GetPostClassifyState(), GetPostClassifyStatus())
            .WillOnce(Return(TError{TError::EType::Logic} << "this is error"));

        NScenarios::TScenarioRunResponse scenarioResponse;
        EXPECT_CALL(scenario, FinishRun(_, _)).WillOnce(Return(scenarioResponse));

        NMegamind::TMegamindAnalyticsInfoBuilder dummyAnalytics;
        dummyAnalytics.SetWinnerScenarioName("not_winner_scenario");
        auto walkerResponse = scenarioWalker.RunFinalize(runCtx, preClassifyState, request, std::move(dummyAnalytics));

        UNIT_ASSERT(!walkerResponse.Scenarios.empty());
        const auto& response = walkerResponse.Scenarios.front();

        UNIT_ASSERT_C(response.BuilderIfExists(), "RunFinalize should answer with builder");

        UNIT_ASSERT_VALUES_EQUAL(SpeechKitResponseToJson(response.BuilderIfExists()->GetSKRProto())["response"],
                                 JsonFromString(R"({
            "card": {
                "type": "simple_text",
                "text": "Прошу прощения, что-то сломалось. Спросите попозже, пожалуйста."
            },
            "cards": [
                {
                    "type": "simple_text",
                    "text": "Прошу прощения, что-то сломалось. Спросите попозже, пожалуйста."
                }
            ],
            "experiments": {},
            "meta": [
                {
                    "error_type": "logic",
                    "type": "error",
                    "message": "_scenario_: this is error"
                },
                {
                    "error_type": "http",
                    "type": "error",
                    "message": "{\"scenario_name\":\"scenario2\",\"message\":\"\",\"stage\":\"postclassify_stage\"}"
                }
            ],
            "directives": []
        })"));
    }

    Y_UNIT_TEST(RunFinalizeAppHostError) {
        StrictMock<TMockWalkerRequestContextTestWrapper> walkerRequestContextTestWrapper;
        StrictMock<TMockRunWalkerRequestCtx> runCtx;
        EXPECT_CALL(runCtx, PostClassifyState())
            .WillRepeatedly(ReturnRef(walkerRequestContextTestWrapper.GetPostClassifyState()));
        EXPECT_CALL(runCtx, Ctx()).WillRepeatedly(ReturnRef(walkerRequestContextTestWrapper.GetCtx()));
        EXPECT_CALL(runCtx, RequestCtx())
            .WillRepeatedly(ReturnRef(walkerRequestContextTestWrapper.Get().RequestCtx()));
        EXPECT_CALL(runCtx, Rng())
            .WillRepeatedly(ReturnRef(walkerRequestContextTestWrapper.GetGlobalCtxWrapper().GetRng()));

        EXPECT_CALL(walkerRequestContextTestWrapper.GetCtx(), Language()).WillRepeatedly(Return(ELanguage::LANG_RUS));

        NiceMock<TMockResponses> responses{};
        TSearchResponse searchObject("", "", TRTLogger::NullLogger(),
                                     /* initDataSources= */ false);
        EXPECT_CALL(responses, WebSearchResponse(_)).WillRepeatedly(ReturnRef(searchObject));
        EXPECT_CALL(walkerRequestContextTestWrapper.GetCtx(), Responses()).WillRepeatedly(ReturnRef(responses));


        auto skr = TSpeechKitRequestBuilder(TSpeechKitRequestBuilder::EPredefined::MinimalWithTextEvent).Build();
        auto request = CreateRequest(IEvent::CreateEvent(skr.Event()), skr);

        TCommonScenarioWalker scenarioWalker(walkerRequestContextTestWrapper.Get().GlobalCtx());

        TScenarioConfig scenarioConfig;
        scenarioConfig.SetName(TEST_SCENARIO_NAME);
        StrictMock<TMockProtocolScenario> scenario{scenarioConfig};

        TSemanticFrame frame;
        *frame.MutableName() = "frame_name";
        const IScenarioWrapper::TSemanticFrames requestFrames = {frame};

        auto ahCtx = NAlice::NMegamind::NTesting::TTestAppHostCtx{walkerRequestContextTestWrapper.Get().GlobalCtx()};
        EXPECT_CALL(runCtx, ItemProxyAdapter()).WillRepeatedly(ReturnRef(ahCtx.ItemProxyAdapter()));
        NMegamind::TGuidGenerator guidGenerator{};
        auto wrapper = MakeIntrusive<TAppHostPureProtocolScenarioWrapper>(
            scenario, walkerRequestContextTestWrapper.GetCtx(), requestFrames, guidGenerator,
            EDeferredApplyMode::DeferApply,
            /* restoreAllFromSession= */ true,
            ahCtx.ItemProxyAdapter());

        IScenarioWalker::TPreClassifyState preClassifyState;
        preClassifyState.ScenarioWrappers = {wrapper};

        EXPECT_CALL(walkerRequestContextTestWrapper.GetPostClassifyState(), GetQualityStorage())
            .WillOnce(Return(TError{TError::EType::NotFound}
                             << "Item \u0027mm_qualitystorage_postclassify\u0027 is not in context"));

        IScenarioWalker::TRunState runState;

        NMegamind::TMegamindAnalyticsInfoBuilder analyticsFromPrepare;
        analyticsFromPrepare.SetWinnerScenarioName(TEST_SCENARIO_NAME);
        auto analyticsFromPrepareClone = analyticsFromPrepare;
        auto walkerResponse =
            scenarioWalker.RunFinalize(runCtx, preClassifyState, request, std::move(analyticsFromPrepareClone));

        UNIT_ASSERT(!walkerResponse.Scenarios.empty());
        const auto& response = walkerResponse.Scenarios.front();

        UNIT_ASSERT_C(response.BuilderIfExists(), "RunFinalize should answer with builder");
        UNIT_ASSERT_VALUES_EQUAL(SpeechKitResponseToJson(response.BuilderIfExists()->GetSKRProto())["response"],
                                 JsonFromString(R"({
            "card": {
              "type": "simple_text",
              "text": "Прошу прощения, что-то сломалось. Спросите попозже, пожалуйста."
            },
            "cards": [
              {
                "type": "simple_text",
                "text": "Прошу прощения, что-то сломалось. Спросите попозже, пожалуйста."
              }
            ],
            "experiments": {},
            "meta": [
              {
                "error_type": "not_found",
                "type": "error",
                "message": ": Item \u0027mm_qualitystorage_postclassify\u0027 is not in context"
              }
            ],
            "directives": []
        })"));

        UNIT_ASSERT_MESSAGES_EQUAL(analyticsFromPrepare.BuildProto(),
                                   walkerResponse.AnalyticsInfoBuilder.BuildProto());
    }

    Y_UNIT_TEST(RestoreWinnerWithContinueResponse) {
        StrictMock<TMockWalkerRequestContextTestWrapper> walkerRequestContextTestWrapper;
        auto skr = TSpeechKitRequestBuilder(TSpeechKitRequestBuilder::EPredefined::MinimalWithTextEvent).Build();
        auto request = CreateRequest(IEvent::CreateEvent(skr.Event()), skr);

        TScenarioConfig scenarioConfig;
        scenarioConfig.SetName(TEST_SCENARIO_NAME);
        StrictMock<TMockProtocolScenario> scenario{scenarioConfig};

        NScenarios::TScenarioRunResponse scenarioResponse;
        scenarioResponse.MutableContinueArguments();
        EXPECT_CALL(scenario, FinishRun(_, _)).WillOnce(Return(scenarioResponse));

        TSemanticFrame frame;
        *frame.MutableName() = "frame_name";
        const IScenarioWrapper::TSemanticFrames requestFrames = {frame};

        NMegamind::TGuidGenerator guidGenerator{};
        auto wrapper = MakeIntrusive<TAppHostPureProtocolScenarioWrapper>(
            scenario, walkerRequestContextTestWrapper.GetCtx(), requestFrames, guidGenerator,
            EDeferredApplyMode::DeferApply,
            /* restoreAllFromSession= */ true,
            walkerRequestContextTestWrapper.Get().ItemProxyAdapter());

        TWalkerResponse walkerResponse;
        NScenarios::TScenarioContinueResponse continueResponse;
        continueResponse.MutableResponseBody()->MutableLayout()->SetOutputSpeech("kek");
        auto wrapperActual =
            NImpl::RestoreWinner(walkerResponse, TEST_SCENARIO_NAME, {wrapper},
                                 walkerRequestContextTestWrapper.Get().Ctx(), request, continueResponse);
        UNIT_ASSERT_C(!wrapperActual.Error(), *wrapperActual.Error());

        UNIT_ASSERT(!walkerResponse.Scenarios.empty());
        auto& topResponse = walkerResponse.Scenarios.front();
        UNIT_ASSERT(topResponse.ResponseBodyIfExists());
        UNIT_ASSERT_MESSAGES_EQUAL(*topResponse.ResponseBodyIfExists(), continueResponse.GetResponseBody());
    }

    Y_UNIT_TEST(ModifyApplyScenarioResponseError) {
        StrictMock<TMockWalkerRequestContextTestWrapper> walkerCtxWrapper;

        TCommonScenarioWalker scenarioWalker(walkerCtxWrapper.Get().GlobalCtx());
        TMockApplyWalkerRequestCtx walkerCtx;

        const auto result =
            scenarioWalker.ModifyApplyScenarioResponse(walkerCtx, TError{TError::EType::Logic} << "logic error");

        UNIT_ASSERT(result.Defined());
    }

    Y_UNIT_TEST(ModifyApplyScenarioResponseSuccess) {
        StrictMock<TMockWalkerRequestContextTestWrapper> walkerCtxWrapper;

        TCommonScenarioWalker scenarioWalker(walkerCtxWrapper.Get().GlobalCtx());
        TMockApplyWalkerRequestCtx walkerCtx;
        TMockContext ctx;
        EXPECT_CALL(Const(walkerCtx), Ctx()).WillRepeatedly(ReturnRef(ctx));
        EXPECT_CALL(walkerCtx, Ctx()).WillRepeatedly(ReturnRef(ctx));

        IScenarioWalker::TApplyState applyState;
        applyState.ActionEffect.Status = IScenarioWalker::TActionEffect::EStatus::WalkerResponseIsNotComplete;

        const auto skr = TSpeechKitRequestBuilder{TSpeechKitRequestBuilder::EPredefined::MinimalWithTextEvent}.Build();
        applyState.Request = CreateRequest(IEvent::CreateEvent(skr.Event()), skr);

        TWalkerResponse walkerResponse;
        TScenarioResponse scenarioResponse{/* scenarioName= */ TEST_SCENARIO_NAME, /* scenarioSemanticFrames= */ {},
                                           /* scenarioAcceptsAnyUtterance= */ false};

        NScenarios::TScenarioResponseBody responseBody;
        responseBody.MutableLayout()->SetOutputSpeech("kek");
        scenarioResponse.SetResponseBody(responseBody);
        walkerResponse.AddScenarioResponse(std::move(scenarioResponse));

        auto wrapper = MakeIntrusive<NiceMock<TMockScenarioWrapper>>();
        EXPECT_CALL(*wrapper, FinishApply).WillOnce(Return(EApplyResult::Called));
        ;

        applyState.ScenarioWrapper = wrapper;
        applyState.WalkerResponse = std::move(walkerResponse);

        StrictMock<NMegamind::NTesting::TMockModifierRequestFactory> modifierRequestFactory;
        EXPECT_CALL(walkerCtx, ModifierRequestFactory).WillRepeatedly(ReturnRef(modifierRequestFactory));
        EXPECT_CALL(modifierRequestFactory, SetupModifierRequest);

        const auto result = scenarioWalker.ModifyApplyScenarioResponse(walkerCtx, std::move(applyState));

        UNIT_ASSERT_C(!result.Defined(), *result);
    }

    Y_UNIT_TEST(ModifyApplyScenarioResponseScenarioFailure) {
        StrictMock<TMockWalkerRequestContextTestWrapper> walkerCtxWrapper;

        TCommonScenarioWalker scenarioWalker(walkerCtxWrapper.Get().GlobalCtx());
        TMockApplyWalkerRequestCtx walkerCtx;
        TMockContext ctx;
        EXPECT_CALL(Const(walkerCtx), Ctx()).WillRepeatedly(ReturnRef(ctx));
        EXPECT_CALL(walkerCtx, Ctx()).WillRepeatedly(ReturnRef(ctx));

        IScenarioWalker::TApplyState applyState;
        applyState.ActionEffect.Status = IScenarioWalker::TActionEffect::EStatus::WalkerResponseIsNotComplete;

        const auto skr = TSpeechKitRequestBuilder{TSpeechKitRequestBuilder::EPredefined::MinimalWithTextEvent}.Build();
        applyState.Request = CreateRequest(IEvent::CreateEvent(skr.Event()), skr);

        TWalkerResponse walkerResponse;
        TScenarioResponse scenarioResponse{/* scenarioName= */ TEST_SCENARIO_NAME, /* scenarioSemanticFrames= */ {},
                                           /* scenarioAcceptsAnyUtterance= */ false};

        NScenarios::TScenarioResponseBody responseBody;
        responseBody.MutableLayout()->SetOutputSpeech("kek");
        scenarioResponse.SetResponseBody(responseBody);
        walkerResponse.AddScenarioResponse(std::move(scenarioResponse));

        auto wrapper = MakeIntrusive<NiceMock<TMockScenarioWrapper>>();
        EXPECT_CALL(*wrapper, FinishApply).WillOnce(Return(TError{TError::EType::NotFound}));

        applyState.ScenarioWrapper = wrapper;
        applyState.WalkerResponse = std::move(walkerResponse);

        StrictMock<NMegamind::NTesting::TMockModifierRequestFactory> modifierRequestFactory;
        EXPECT_CALL(walkerCtx, ModifierRequestFactory).WillRepeatedly(ReturnRef(modifierRequestFactory));

        const auto result = scenarioWalker.ModifyApplyScenarioResponse(walkerCtx, std::move(applyState));

        UNIT_ASSERT(result.Defined());
    }

    class TMockFrameRequestProcessor : public IFrameRequestProcessor {
    public:
        MOCK_METHOD(void, Process, (const TTypedSemanticFrameRequest&), (override));
    };

    struct TProcessActionEffectFixture : public NUnitTest::TBaseFixture {
        IScenarioWalker::TActionEffect ActionEffect;
        IScenarioWalker::TRunState RunState;
        StrictMock<TMockFrameRequestProcessor> FrameRequestProcessorMock;
        bool UtteranceWasUpdated = false;
        std::unique_ptr<const IEvent> Event;
        TVector<TSemanticFrame> ForcedSemanticFrames;
        TVector<TSemanticFrame> RecognizedActionEffectFrames;

        TProcessActionEffectFixture() {
            ActionEffect.Status = IScenarioWalker::TActionEffect::EStatus::WalkerResponseIsNotComplete;
        }

        bool ProcessActionEffect(bool doNotForceActionEffectFrameWhenNoUtteranceUpdate = false) {
            return NImpl::ProcessActionEffect(
                ActionEffect, RunState, FrameRequestProcessorMock, UtteranceWasUpdated, Event,
                RecognizedActionEffectFrames, ForcedSemanticFrames, TRTLogger::NullLogger(),
                doNotForceActionEffectFrameWhenNoUtteranceUpdate
            );
        }

        bool ProcessActionEffectWithExp() {
            return ProcessActionEffect(/* doNotForceActionEffectFrameWhenNoUtteranceUpdate= */ true);
        }
    };

    Y_UNIT_TEST_F(ProcessActionEffect_givenNonEmptyEffectFrame_setsItAsOnlyRecognizedActionAndForcedSemanticFrame, TProcessActionEffectFixture) {
        ActionEffect.EffectFrame = TSemanticFrameBuilder("some_frame").Build();
        UNIT_ASSERT(ProcessActionEffect());
        UNIT_ASSERT_EQUAL(RecognizedActionEffectFrames.size(), 1);
        UNIT_ASSERT_MESSAGES_EQUAL(RecognizedActionEffectFrames[0], *ActionEffect.EffectFrame);
        UNIT_ASSERT_EQUAL(ForcedSemanticFrames.size(), 1);
        UNIT_ASSERT_MESSAGES_EQUAL(ForcedSemanticFrames[0], *ActionEffect.EffectFrame);
    }

    Y_UNIT_TEST_F(ProcessActionEffectWithExp_givenNonEmptyEffectFrame_setsItAsOnlyRecognizedAction, TProcessActionEffectFixture) {
        ActionEffect.EffectFrame = TSemanticFrameBuilder("some_frame").Build();
        UNIT_ASSERT(ProcessActionEffectWithExp());
        UNIT_ASSERT_EQUAL(RecognizedActionEffectFrames.size(), 1);
        UNIT_ASSERT_MESSAGES_EQUAL(RecognizedActionEffectFrames[0], *ActionEffect.EffectFrame);
        UNIT_ASSERT(ForcedSemanticFrames.empty());
    }

    TSemanticFrameRequestData MakeSemanticFrameRequestData(const TString& query = "some query") {
        TSemanticFrameRequestData sfrData;
        sfrData.MutableTypedSemanticFrame()->MutableSearchSemanticFrame()->MutableQuery()->SetStringValue(query);
        sfrData.MutableAnalytics()->SetOrigin(TAnalyticsTrackingModule::Scenario);
        sfrData.MutableAnalytics()->SetProductScenario("search");
        sfrData.MutableAnalytics()->SetPurpose("get_factoid");
        return sfrData;
    }

    Y_UNIT_TEST_F(ProcessActionEffect_givenNonEmptyActionFramesOfSize1_passesActionFrameToFrameRequestProcessor, TProcessActionEffectFixture) {
        const TSemanticFrameRequestData sfrData = MakeSemanticFrameRequestData();
        ActionEffect.ActionFrames = {sfrData};
        EXPECT_CALL(FrameRequestProcessorMock, Process(TTypedSemanticFrameRequest{sfrData})).Times(1);
        UNIT_ASSERT(ProcessActionEffect());
        UNIT_ASSERT(RecognizedActionEffectFrames.empty());
        UNIT_ASSERT(ForcedSemanticFrames.empty());
    }

    Y_UNIT_TEST_F(ProcessActionEffectWithExp_givenNonEmptyActionFramesOfSize1_passesActionFrameToFrameRequestProcessor, TProcessActionEffectFixture) {
        const TSemanticFrameRequestData sfrData = MakeSemanticFrameRequestData();
        ActionEffect.ActionFrames = {sfrData};
        EXPECT_CALL(FrameRequestProcessorMock, Process(TTypedSemanticFrameRequest{sfrData})).Times(1);
        UNIT_ASSERT(ProcessActionEffectWithExp());
        UNIT_ASSERT(RecognizedActionEffectFrames.empty());
        UNIT_ASSERT(ForcedSemanticFrames.empty());
    }

    Y_UNIT_TEST_F(ProcessActionEffect_givenNonEmptyActionFramesOfSize2_addsActionFramesToRecognizedActionEffectFramesAndForcedSemanticFrames, TProcessActionEffectFixture) {
        const TSemanticFrameRequestData firstSfrData = MakeSemanticFrameRequestData("first query");
        const TSemanticFrameRequestData secondSfrData = MakeSemanticFrameRequestData("second query");
        ActionEffect.ActionFrames = {firstSfrData, secondSfrData};
        UNIT_ASSERT(ProcessActionEffect());
        UNIT_ASSERT_EQUAL(RecognizedActionEffectFrames.size(), 2);
        UNIT_ASSERT_MESSAGES_EQUAL(RecognizedActionEffectFrames[0], TTypedSemanticFrameRequest{firstSfrData}.SemanticFrame);
        UNIT_ASSERT_MESSAGES_EQUAL(RecognizedActionEffectFrames[1], TTypedSemanticFrameRequest{secondSfrData}.SemanticFrame);
        UNIT_ASSERT_EQUAL(ForcedSemanticFrames.size(), 2);
        UNIT_ASSERT_MESSAGES_EQUAL(ForcedSemanticFrames[0], TTypedSemanticFrameRequest{firstSfrData}.SemanticFrame);
        UNIT_ASSERT_MESSAGES_EQUAL(ForcedSemanticFrames[1], TTypedSemanticFrameRequest{secondSfrData}.SemanticFrame);
    }

    Y_UNIT_TEST_F(ProcessActionEffectWithExp_givenNonEmptyActionFramesOfSize2_addsActionFramesToRecognizedActionEffectFrames, TProcessActionEffectFixture) {
        const TSemanticFrameRequestData firstSfrData = MakeSemanticFrameRequestData("first query");
        const TSemanticFrameRequestData secondSfrData = MakeSemanticFrameRequestData("second query");
        ActionEffect.ActionFrames = {firstSfrData, secondSfrData};
        UNIT_ASSERT(ProcessActionEffectWithExp());
        UNIT_ASSERT_EQUAL(RecognizedActionEffectFrames.size(), 2);
        UNIT_ASSERT_MESSAGES_EQUAL(RecognizedActionEffectFrames[0], TTypedSemanticFrameRequest{firstSfrData}.SemanticFrame);
        UNIT_ASSERT_MESSAGES_EQUAL(RecognizedActionEffectFrames[1], TTypedSemanticFrameRequest{secondSfrData}.SemanticFrame);
        UNIT_ASSERT(ForcedSemanticFrames.empty());
    }

    Y_UNIT_TEST(RestoreInitializedWrappers) {
        StrictMock<TMockWalkerRequestContextTestWrapper> walkerRequestContextTestWrapper;
        TVector<TSimpleSharedPtr<TConfigBasedAppHostPureProtocolScenario>> globalScenarios;

        auto generateWrappers = [&](const TVector<TString>& names) {
            TScenarioWrapperPtrs wrappers;
            for (const auto& name : names) {
                TScenarioConfig config;
                config.SetName(name);
                TConfigBasedAppHostPureProtocolScenario scenario{config};
                globalScenarios.push_back(MakeSimpleShared<TConfigBasedAppHostPureProtocolScenario>(std::move(scenario)));
                auto wrapper = MakeIntrusive<NiceMock<TMockScenarioWrapper>>();
                EXPECT_CALL(*wrapper, GetScenario()).WillRepeatedly(ReturnRef(*globalScenarios.back()));
                wrappers.push_back(wrapper);
            }
            return wrappers;
        };

        struct TTestCase {
            TVector<TString> Wrappers, LaunchedWrappers, ExpectedInitialized;
        };

        TVector<TTestCase> cases;
        cases.push_back({.Wrappers = {"a", "c", "b", "d"},
                         .LaunchedWrappers = {"d", "c"},
                         .ExpectedInitialized = {"c", "d"}});
        cases.push_back({.Wrappers = {"a", "x", "b", "d", "c"},
                         .LaunchedWrappers = {"d", "c", "e"},
                         .ExpectedInitialized = {"d", "c"}});
        cases.push_back({.Wrappers = {"a"},
                         .LaunchedWrappers = {"a"},
                         .ExpectedInitialized = {"a"}});
        cases.push_back({.Wrappers = {"b"},
                         .LaunchedWrappers = {"a"},
                         .ExpectedInitialized = {}});
        cases.push_back({.Wrappers = {"a", "z"},
                         .LaunchedWrappers = {"z", "a", "x"},
                         .ExpectedInitialized = {"a", "z"}});

        for (const auto& testCase : cases) {
            TScenarioWrapperPtrs wrappers = generateWrappers(testCase.Wrappers);
            NMegamindAppHost::TLaunchedScenarios launchedScenarios;
            for (const auto& launchedWrapper : testCase.LaunchedWrappers) {
                launchedScenarios.AddScenarios()->SetName(launchedWrapper);
            }
            auto ahCtx =
                NAlice::NMegamind::NTesting::TTestAppHostCtx{walkerRequestContextTestWrapper.Get().GlobalCtx()};
            ahCtx.TestCtx().AddProtobufItem(launchedScenarios, NMegamind::AH_ITEM_LAUNCHED_SCENARIOS,
                                            NAppHost::EContextItemKind::Input);
            auto initializedWrappers = NImpl::RestoreInitializedWrappers(ahCtx.ItemProxyAdapter(), wrappers);
            UNIT_ASSERT_C(initializedWrappers.IsSuccess(), *initializedWrappers.Error());
            TVector<TString> actualNames;
            for (const auto& wrapper : initializedWrappers.Value()) {
                actualNames.push_back(wrapper->GetScenario().GetName());
            }
            UNIT_ASSERT_VALUES_EQUAL(actualNames, testCase.ExpectedInitialized);
        }
    }
}

Y_UNIT_TEST_SUITE(WalkerWriteMetrics) {
    Y_UNIT_TEST(PreProcessGetNextWithInvalidSessionId) {
        NMegamind::TStackEngine stackEngine{};
        stackEngine.StartNewSession("session_id", "product_scenario_name", "scenario_name");
        stackEngine.Push([]{
            NMegamind::TStackEngine::TItem item{};
            item.SetScenarioName(TEST_SCENARIO_NAME);
            return item;
        }());
        StrictMock<TMockSensors> sensors;
        EXPECT_CALL(sensors, IncRate(NMonitoring::TLabels{
            {"scenario_name", TEST_SCENARIO_NAME},
            {"name", "stack_engine.errors_per_second"},
            {"error_type", "invalid_session"}
        }));
        TestPreProcessRequestGetNextWriteMetrics(stackEngine.GetCore(), sensors);
    }

    Y_UNIT_TEST(PreProcessGetNextWithInvalidEffect) {
        NMegamind::TStackEngine stackEngine{};
        stackEngine.Push([]{
            NMegamind::TStackEngine::TItem item{};
            item.SetScenarioName(TEST_SCENARIO_NAME);
            return item;
        }());
        StrictMock<TMockSensors> sensors;
        EXPECT_CALL(sensors, IncRate(NMonitoring::TLabels{
            {"scenario_name", TEST_SCENARIO_NAME},
            {"name", "stack_engine.errors_per_second"},
            {"error_type", "invalid_effect"}
        }));
        TestPreProcessRequestGetNextWriteMetrics(stackEngine.GetCore(), sensors);
    }

    Y_UNIT_TEST(PreProcessGetNextRps) {
        NMegamind::TStackEngine stackEngine{};
        for (int i = 0; i < 3; ++i) {
            stackEngine.Push(NMegamind::TStackEngine::TItem{});
        }
        stackEngine.Push(MakeStackEngineItemWithSearchParsedUtterance());
        StrictMock<TMockSensors> sensors;
        EXPECT_CALL(sensors, IncRate(NMonitoring::TLabels{
            {"scenario_name", TEST_SCENARIO_NAME},
            {"name", "stack_engine.requests_per_second"}
        }));
        EXPECT_CALL(sensors, AddHistogram(NMonitoring::TLabels{
                                            {"scenario_name", TEST_SCENARIO_NAME},
                                            {"name", "stack_engine.stack_size"}},
                                          4,
                                          NMetrics::SMALL_SIZE_INTERVALS));
        TestPreProcessRequestGetNextWriteMetrics(stackEngine.GetCore(), sensors);
    }

    Y_UNIT_TEST(PreProcessGetNextRpsWithInvalidEffect) {
        NMegamind::TStackEngine stackEngine{};
        for (int i = 0; i < 3; ++i) {
            stackEngine.Push(NMegamind::TStackEngine::TItem{});
        }
        stackEngine.Push(MakeStackEngineItemWithSearchParsedUtterance());
        stackEngine.Push([]{
            NMegamind::TStackEngine::TItem item{};
            item.SetScenarioName(TEST_SCENARIO_NAME);
            return item;
        }());

        StrictMock<TMockSensors> sensors;
        EXPECT_CALL(sensors, IncRate(NMonitoring::TLabels{
            {"scenario_name", TEST_SCENARIO_NAME},
            {"name", "stack_engine.requests_per_second"}
        }));
        EXPECT_CALL(sensors, AddHistogram(NMonitoring::TLabels{
                                                  {"scenario_name", TEST_SCENARIO_NAME},
                                                  {"name", "stack_engine.stack_size"}},
                                          4,
                                          NMetrics::SMALL_SIZE_INTERVALS));
        EXPECT_CALL(sensors, IncRate(NMonitoring::TLabels{
            {"scenario_name", TEST_SCENARIO_NAME},
            {"name", "stack_engine.errors_per_second"},
            {"error_type", "invalid_effect"}
        }));
        TestPreProcessRequestGetNextWriteMetrics(stackEngine.GetCore(), sensors);
    }
}

Y_UNIT_TEST_SUITE(FramesToScenarioMapping) {
    Y_UNIT_TEST(AdditionalFramesSubscriptionByExperiment) {
        StrictMock<TMockWalkerRequestContextTestWrapper> walkerCtxWrapper;
        TCommonScenarioWalker scenarioWalker(walkerCtxWrapper.Get().GlobalCtx());

        TMockContext ctx;
        EXPECT_CALL(ctx, Logger()).WillRepeatedly(ReturnRef(walkerCtxWrapper.GetCtx().Logger()));

        THashMap<TString, TMaybe<TString>> flags {
                {"mm_subscribe_to_frame=ScenarioName1:exp_frame.a", "1"},
                {"mm_subscribe_to_frame=ScenarioName2:exp_frame.a,exp_frame.b", "1"},
        };
        EXPECT_CALL(ctx, ExpFlags()).WillRepeatedly(ReturnRef(flags));

        TMockLightWalkerRequestCtx walkerCtx;
        EXPECT_CALL(walkerCtx, Ctx()).WillRepeatedly(ReturnRef(ctx));
        TScenarioConfigRegistry scenarioConfigRegistry{};
        scenarioConfigRegistry.AddScenarioConfig([&] {
            TScenarioConfig scenarioConfig{};
            scenarioConfig.SetName("ScenarioName1");
            scenarioConfig.AddAcceptedFrames("frame.a");
            return scenarioConfig;
        }());
        scenarioConfigRegistry.AddScenarioConfig([&] {
            TScenarioConfig scenarioConfig{};
            scenarioConfig.SetName("ScenarioName2");
            scenarioConfig.AddAcceptedFrames("frame.b");
            return scenarioConfig;
        }());
        scenarioConfigRegistry.AddScenarioConfig([&] {
            TScenarioConfig scenarioConfig{};
            scenarioConfig.SetName("ScenarioName3");
            scenarioConfig.AddAcceptedFrames("frame.c");
            return scenarioConfig;
        }());
        ON_CALL(walkerCtx, ScenarioConfigRegistry()).WillByDefault(ReturnRef(scenarioConfigRegistry));
        auto ahCtx = NAlice::NMegamind::NTesting::TTestAppHostCtx{walkerCtxWrapper.Get().GlobalCtx()};
        NMegamind::TItemProxyAdapter& itemProxyAdapter = ahCtx.ItemProxyAdapter();
        EXPECT_CALL(walkerCtx, ItemProxyAdapter()).WillRepeatedly(ReturnRef(itemProxyAdapter));

        const auto& speechKitRequest = walkerCtxWrapper.GetCtx().SpeechKitRequest();
        auto request = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);

        auto scenarioToRequestFrames = scenarioWalker.ScenarioToRequestFrames(walkerCtx, request);

        const THashMap<TString, TSet<TString>> scenarioToExpectedFrames {
            {"ScenarioName1", {"frame.a", "exp_frame.a"}},
            {"ScenarioName2", {"frame.b", "exp_frame.a", "exp_frame.b"}},
            {"ScenarioName3", {"frame.c"}},
        };

        for (const auto& [scenarioRef, frames] : scenarioToRequestFrames) {
            const auto& scenarioName = scenarioRef->GetScenario().GetName();
            auto* expectedFramesPtr = MapFindPtr(scenarioToExpectedFrames, scenarioName);
            UNIT_ASSERT(expectedFramesPtr);
            UNIT_ASSERT_VALUES_EQUAL(frames.size(), expectedFramesPtr->size());
            for (const auto& frame : frames) {
                UNIT_ASSERT_C(expectedFramesPtr->contains(frame.GetName()),
                              "Scenario " << scenarioName << "accepts unexpected frame " << frame.GetName());
            }
        }
    }
}

Y_UNIT_TEST_SUITE(ConditionalDatasources) {
    Y_UNIT_TEST(TestPushFlagsForConditionalDatasources) {
        const TString scenarioName = "ScenarioName";
        const TString frameName = "frame_name";
        const TString flagName = "need_conditional_datasource_" + scenarioName + "_" + "WEB_SEARCH_DOCS";

        NiceMock<TMockGlobalContext> globalCtx{};

        TMockContext ctx;
        NAlice::TConfig_TScenarios_TConfig scenarioConfig;
        auto& cds = *scenarioConfig.AddConditionalDataSources();
        cds.SetDataSourceType(EDataSourceType::WEB_SEARCH_DOCS);
        cds.AddConditions()->MutableOnSemanticFrameCondition()->SetSemanticFrameName(frameName);
        cds.AddConditions()->MutableOnExperimentFlag()->SetFlagName("exp_flag_name");
        cds.AddConditions()->MutableOnUserLanguage()->SetLanguage(ELang::L_ARA);
        EXPECT_CALL(ctx, ScenarioConfig(_)).WillRepeatedly(ReturnRef(scenarioConfig));
        EXPECT_CALL(ctx, HasExpFlag("exp_flag_name")).WillRepeatedly(Return(false));

        TScenarioConfig config;
        config.SetName(scenarioName);
        auto scenario = MakeSimpleShared<TConfigBasedAppHostPureProtocolScenario>(config);
        auto wrapper = MakeIntrusive<NiceMock<TMockScenarioWrapper>>();
        EXPECT_CALL(*wrapper, GetScenario()).WillRepeatedly(Invoke([scenario]()->const TScenario& { return *scenario; }));
        TScenarioWrapperPtrs wrappers{
            wrapper
        };

        {
            NMegamind::NTesting::TTestAppHostCtx ahCtx{globalCtx};
            TSemanticFrame frame;
            frame.SetName(frameName);
            TVector<TSemanticFrame> frames = {frame};
            EXPECT_CALL(*wrapper, GetSemanticFrames()).WillRepeatedly(ReturnRef(frames));
            NImpl::PushFlagsForConditionalDatasources(ahCtx.ItemProxyAdapter(), ctx, wrappers);
            UNIT_ASSERT(ahCtx.TestCtx().HasProtobufItem(flagName));
        }
        {
            NMegamind::NTesting::TTestAppHostCtx ahCtx{globalCtx};
            TSemanticFrame frame;
            frame.SetName("another_frame");
            TVector<TSemanticFrame> frames = {frame};
            EXPECT_CALL(*wrapper, GetSemanticFrames()).WillRepeatedly(ReturnRef(frames));
            NImpl::PushFlagsForConditionalDatasources(ahCtx.ItemProxyAdapter(), ctx, wrappers);
            UNIT_ASSERT(!ahCtx.TestCtx().HasProtobufItem(flagName));
        }
        {
            NMegamind::NTesting::TTestAppHostCtx ahCtx{globalCtx};
            TSemanticFrame frame;
            frame.SetName("another_frame");
            TVector<TSemanticFrame> frames = {frame};
            EXPECT_CALL(*wrapper, GetSemanticFrames()).WillRepeatedly(ReturnRef(frames));
            EXPECT_CALL(ctx, HasExpFlag("exp_flag_name")).WillRepeatedly(Return(true));
            NImpl::PushFlagsForConditionalDatasources(ahCtx.ItemProxyAdapter(), ctx, wrappers);
            EXPECT_CALL(ctx, HasExpFlag("exp_flag_name")).WillRepeatedly(Return(false));
            UNIT_ASSERT(ahCtx.TestCtx().HasProtobufItem(flagName));
        }
        {
            NMegamind::NTesting::TTestAppHostCtx ahCtx{globalCtx};
            TSemanticFrame frame;
            frame.SetName("another_frame");
            TVector<TSemanticFrame> frames = {frame};
            EXPECT_CALL(*wrapper, GetSemanticFrames()).WillRepeatedly(ReturnRef(frames));
            EXPECT_CALL(ctx, Language()).WillRepeatedly(Return(ELanguage::LANG_ARA));
            NImpl::PushFlagsForConditionalDatasources(ahCtx.ItemProxyAdapter(), ctx, wrappers);
            UNIT_ASSERT(ahCtx.TestCtx().HasProtobufItem(flagName));
        }
    }
}

} // namespace NAlice
