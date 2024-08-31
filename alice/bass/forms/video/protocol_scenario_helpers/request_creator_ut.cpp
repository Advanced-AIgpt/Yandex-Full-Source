#include "request_creator.h"

#include <alice/bass/ut/helpers.h>

#include <alice/library/json/json.h>
#include <alice/library/video_common/defs.h>

#include <library/cpp/protobuf/json/proto2json.h>

using namespace NAlice::NScenarios;

namespace {

constexpr TStringBuf REQUEST_PROTOBUF_META = TStringBuf(R"(
{
    "base_request": {
        "device_state": {
            "timers": {
                "active_timers": [
                    {
                        "start_timestamp": 12323434545656789
                    }
                ]
            }
        },
        "client_info": {
            "app_id": "ru.yandex.quasar.app/1.0",
            "timestamp": "1234"
        },
        "random_seed": 1234512345
    },
    "input": {
        "semantic_frames": [
            {
                "name": "personal_assistant.scenarios.video_play",
                "slots" : [
                    {
                        "name": "film_genre",
                        "type": "video_film_genre",
                        "value": "comedy"
                    }
                ]
            }
        ]
    }
}
)");

const NSc::TValue RESULT_RUN_REQUEST = NSc::TValue::FromJson(R"'(
{
  "form": {
    "name": "personal_assistant.scenarios.video_play",
    "slots": [
      {
        "name": "film_genre",
        "optional": 1,
        "type": "video_film_genre",
        "value": "comedy"
      },
      {
        "name": "calculate_video_factors_on_bass",
        "optional": 1,
        "type": "bool",
        "value": "true"
      }
    ]
  },
  "meta": {
    "client_features": {
      "supported":[]
    },
    "client_id": "ru.yandex.quasar.app/1.0/ ( ;  )",
    "client_info": {
      "app_id": "ru.yandex.quasar.app/1.0",
      "app_version": "",
      "device_manufacturer": "",
      "device_model": "",
      "os_version": "",
      "platform": ""
    },
    "client_ip": "",
    "device_id": "",
    "device_state": {
      "timers": {
        "active_timers": [
          {
            "start_timestamp": 12323434545656789
          }
        ]
      }
    },
    "dialog_id": "",
    "epoch": 1234,
    "experiments": null,
    "is_porn_query": 0,
    "lang": "",
    "location": {
      "accuracy": 0,
      "lat": 0,
      "lon": 0,
      "recency": 0
    },
    "megamind_cookies" : "{}",
    "request_id": "",
    "rng_seed": "1234512345",
    "tz": "",
    "uuid": "",
    "user_agent": "",
    "voice_session": 0
  }
}
)'");

constexpr TStringBuf REQUEST_PROTOBUF_MALFORMED_META = TStringBuf(R"(
{
    "base_request": {
        "device_state": {
            "video": {
                "current_screen": "video_player",
                "last_play_timestamp": 1613272002461
            }
        },
        "experiments": {
            "video_irrel_if_device_state_malformed": "1"
        },
        "client_info": {
            "app_id": "ru.yandex.quasar.app/1.0",
            "timestamp": "1234"
        },
        "random_seed": 1234612346
    },
    "input": {
        "semantic_frames": [
            {
                "name": "personal_assistant.scenarios.quasar.open_current_video",
                "slots" : [
                    {
                        "name": "action",
                        "type": "custom.video_selection_action",
                        "value": "play"
                    }
                ]
            }
        ]
    }
}
)");

NBASS::TResultValue RunTest(TStringBuf request, TStringBuf intent, NSc::TValue& resultRequest) {
    TScenarioRunRequest requestProto;
    NAlice::JsonToProto(NAlice::JsonFromString(request), requestProto);
    NAlice::NVideoCommon::TVideoFeatures features;
    TMaybe<TString> searchText{};
    return NVideoProtocol::CreateBassRunVideoRequest(requestProto, resultRequest, features, intent, searchText);
}

Y_UNIT_TEST_SUITE(ProtocolScenarioRequestMaker) {
    Y_UNIT_TEST(ParsingSimpleMeta) {
        NSc::TValue resultRequest;
        const auto err = RunTest(REQUEST_PROTOBUF_META, NAlice::NVideoCommon::SEARCH_VIDEO, resultRequest);

        UNIT_ASSERT_C(!err.Defined(), *err);
        UNIT_ASSERT(NTestingHelpers::EqualJson(resultRequest, RESULT_RUN_REQUEST));

    }

    Y_UNIT_TEST(ParsingMalformedDeviceState) {
        NSc::TValue resultRequest;
        const auto err = RunTest(REQUEST_PROTOBUF_MALFORMED_META, NAlice::NVideoCommon::QUASAR_OPEN_CURRENT_VIDEO, resultRequest);
        UNIT_ASSERT(err.Defined());
        UNIT_ASSERT_C(err.Get()->Type == NBASS::TError::EType::PROTOCOL_IRRELEVANT, TStringBuilder() << "Unexpected error type: " << err.Get()->Type);
        UNIT_ASSERT_C(err.Get()->Msg.StartsWith("Device state malformed for intent"), TStringBuilder() << "Unexpected error message: " << err.Get()->Msg);
}
}

} // namespace
