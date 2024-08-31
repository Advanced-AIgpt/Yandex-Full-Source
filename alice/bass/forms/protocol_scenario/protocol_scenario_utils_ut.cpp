#include "protocol_scenario_utils.h"

#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/ut/helpers.h>

#include <alice/library/json/json.h>
#include <alice/library/proto/protobuf.h>
#include <alice/library/util/variant.h>

#include <library/cpp/protobuf/json/proto2json.h>
#include <library/cpp/scheme/scheme.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice;
using namespace NAlice::NScenarios;
using namespace NBASS;
using namespace NTestingHelpers;

namespace {
constexpr TStringBuf VIDEO_PLAY_RESPONSE = TStringBuf(R"(
{
  "blocks": [
    {
      "command_sub_type": "video_play",
      "command_type": "video_play",
      "data": {
        "item": {
          "cover_url_16x9": "http://avatars.mds.yandex.net/get-ott/239697/2a000001609d11b5ab628418bae03a8fb9b9",
          "episode": 1,
          "episodes_count": 18,
          "human_readable_id": "",
          "min_age": 0,
          "name": "Ну, погоди! - Сезон 1 - Серия 1 - Первый выпуск",
          "provider_info": [
            {
              "episode": 1,
              "human_readable_id": "",
              "provider_item_id": "4ae1ba2934cb649084b8de237330ea58",
              "provider_name": "kinopoisk",
              "provider_number": 1,
              "season": 1,
              "tv_show_item_id": "4ab7044baa2c2dc599c244196429245f",
              "tv_show_season_id": "43c82f93c8c484f0947a51ef0f360115",
              "type": "tv_show_episode"
            }
          ],
          "provider_item_id": "4ae1ba2934cb649084b8de237330ea58",
          "provider_name": "kinopoisk",
          "provider_number": 1,
          "rating": 8.687000275,
          "season": 1,
          "seasons_count": 1,
          "soon": 0,
          "thumbnail_url_16x9": "http://avatars.mds.yandex.net/get-ott/239697/2a000001609d11b5ab628418bae03a8fb9b9",
          "tv_show_item_id": "4ab7044baa2c2dc599c244196429245f",
          "tv_show_season_id": "43c82f93c8c484f0947a51ef0f360115",
          "type": "tv_show_episode"
        },
        "next_item": {
          "cover_url_16x9": "http://avatars.mds.yandex.net/get-ott/239697/2a000001609d11beec76de64c88f89902c18",
          "cover_url_2x3": "http://avatars.mds.yandex.net/get-ott/223007/2a00000164db8bfab863760baf21b17be1e0",
          "episode": 2,
          "episodes_count": 18,
          "human_readable_id": "",
          "min_age": 0,
          "name": "Ну, погоди! - Сезон 1 - Серия 2 - Второй выпуск",
          "provider_info": [
            {
              "episode": 2,
              "human_readable_id": "",
              "provider_item_id": "4d472df7e7f4ceea81d426c6e2710043",
              "provider_name": "kinopoisk",
              "provider_number": 2,
              "season": 1,
              "tv_show_item_id": "4ab7044baa2c2dc599c244196429245f",
              "tv_show_season_id": "43c82f93c8c484f0947a51ef0f360115",
              "type": "tv_show_episode"
            }
          ],
          "provider_item_id": "4d472df7e7f4ceea81d426c6e2710043",
          "provider_name": "kinopoisk",
          "provider_number": 2,
          "rating": 8.687000275,
          "season": 1,
          "seasons_count": 1,
          "soon": 0,
          "thumbnail_url_16x9": "http://avatars.mds.yandex.net/get-ott/239697/2a000001609d11beec76de64c88f89902c18",
          "tv_show_item_id": "4ab7044baa2c2dc599c244196429245f",
          "tv_show_season_id": "43c82f93c8c484f0947a51ef0f360115",
          "type": "tv_show_episode"
        },
        "payload": "video_play_payload",
        "tv_show_item": {
          "actors": "Клара Румянова, Анатолий Папанов, Геннадий Хазанов",
          "cover_url_16x9": "http://avatars.mds.yandex.net/get-ott/374297/2a0000016513e6f85a22e34b6071666844b9",
          "debug_info": {
            "web_page_url": "http://www.kinopoisk.ru/film/46483"
          },
          "description": "Веселые приключения неразлучной парочки - хулигана Волка и смышленого Зайца",
          "directors": "Вячеслав Котёночкин, Юрий Бутырин, Владимир Тарасов",
          "genre": "мультфильм, комедия, семейный, приключения",
          "human_readable_id": "",
          "min_age": 0,
          "misc_ids": {
            "kinopoisk": "46483"
          },
          "name": "Ну, погоди!",
          "provider_info": [
            {
              "human_readable_id": "",
              "misc_ids": {
                "kinopoisk": "46483"
              },
              "provider_item_id": "4ab7044baa2c2dc599c244196429245f",
              "provider_name": "kinopoisk",
              "type": "tv_show"
            }
          ],
          "provider_item_id": "4ab7044baa2c2dc599c244196429245f",
          "provider_name": "kinopoisk",
          "rating": 8.687000275,
          "release_year": 2006,
          "relevance": 103845624,
          "relevance_prediction": 0.09998173103,
          "seasons_count": 1,
          "type": "tv_show"
        },
        "uri": "some_play_uri"
      },
      "type": "command"
    },
    {
      "attention_type": "video_autoplay",
      "data": null,
      "type": "attention"
    },
    {
      "data": {
        "features": {
          "hdmi_output": {
            "enabled": true
          }
        }
      },
      "type": "client_features"
    }
  ],
  "form": {
    "name": "personal_assistant.scenarios.video_play",
    "slots": [
      {
        "name": "search_text",
        "optional": true,
        "source_text": "ну погоди",
        "type": "string",
        "value": "ну погоди"
      },
      {
        "name": "action",
        "optional": true,
        "source_text": "включи",
        "type": "video_action",
        "value": "play"
      }
    ]
  },
  "meta": {
    "rng_seed": "0"
  }
}
)");

TStringBuf EXPECTED_VIDEO_PROTO_ANSWER = TStringBuf(R"(
{
  "analytics_info": {
    "intent": "mm.personal_assistant.scenarios.video_play"
  },
  "layout": {
    "cards": [
      {
        "text": "Сейчас включу."
      }
    ],
    "output_speech": "Сейчас включу.",
    "directives": [
      {
        "video_play_directive": {
          "name": "video_play",
          "payload" : "video_play_payload",
          "uri": "some_play_uri",
          "item": {
            "type": "tv_show_episode",
            "provider_name": "kinopoisk",
            "provider_item_id": "4ae1ba2934cb649084b8de237330ea58",
            "tv_show_season_id": "43c82f93c8c484f0947a51ef0f360115",
            "tv_show_item_id": "4ab7044baa2c2dc599c244196429245f",
            "episode": 1,
            "season": 1,
            "provider_number": 1,
            "cover_url_16x9": "http://avatars.mds.yandex.net/get-ott/239697/2a000001609d11b5ab628418bae03a8fb9b9",
            "thumbnail_url_16x9": "http://avatars.mds.yandex.net/get-ott/239697/2a000001609d11b5ab628418bae03a8fb9b9",
            "name": "Ну, погоди! - Сезон 1 - Серия 1 - Первый выпуск",
            "rating": 8.687000275,
            "seasons_count": 1,
            "episodes_count": 18,
            "provider_info": [
              {
                "type": "tv_show_episode",
                "provider_name": "kinopoisk",
                "provider_item_id": "4ae1ba2934cb649084b8de237330ea58",
                "tv_show_season_id": "43c82f93c8c484f0947a51ef0f360115",
                "tv_show_item_id": "4ab7044baa2c2dc599c244196429245f",
                "episode": 1,
                "season": 1,
                "provider_number": 1
              }
            ]
          },
          "next_item": {
            "type": "tv_show_episode",
            "provider_name": "kinopoisk",
            "provider_item_id": "4d472df7e7f4ceea81d426c6e2710043",
            "tv_show_season_id": "43c82f93c8c484f0947a51ef0f360115",
            "tv_show_item_id": "4ab7044baa2c2dc599c244196429245f",
            "episode": 2,
            "season": 1,
            "provider_number": 2,
            "cover_url_2x3": "http://avatars.mds.yandex.net/get-ott/223007/2a00000164db8bfab863760baf21b17be1e0",
            "cover_url_16x9": "http://avatars.mds.yandex.net/get-ott/239697/2a000001609d11beec76de64c88f89902c18",
            "thumbnail_url_16x9": "http://avatars.mds.yandex.net/get-ott/239697/2a000001609d11beec76de64c88f89902c18",
            "name": "Ну, погоди! - Сезон 1 - Серия 2 - Второй выпуск",
            "rating": 8.687000275,
            "seasons_count": 1,
            "episodes_count": 18,
            "provider_info": [
              {
                "type": "tv_show_episode",
                "provider_name": "kinopoisk",
                "provider_item_id": "4d472df7e7f4ceea81d426c6e2710043",
                "tv_show_season_id": "43c82f93c8c484f0947a51ef0f360115",
                "tv_show_item_id": "4ab7044baa2c2dc599c244196429245f",
                "episode": 2,
                "season": 1,
                "provider_number": 2
              }
            ]
          },
          "tv_show_item": {
            "type": "tv_show",
            "provider_name": "kinopoisk",
            "provider_item_id": "4ab7044baa2c2dc599c244196429245f",
            "misc_ids": {
              "kinopoisk": "46483"
            },
            "cover_url_16x9": "http://avatars.mds.yandex.net/get-ott/374297/2a0000016513e6f85a22e34b6071666844b9",
            "name": "Ну, погоди!",
            "description": "Веселые приключения неразлучной парочки - хулигана Волка и смышленого Зайца",
            "genre": "мультфильм, комедия, семейный, приключения",
            "rating": 8.687000275,
            "seasons_count": 1,
            "release_year": 2006,
            "directors": "Вячеслав Котёночкин, Юрий Бутырин, Владимир Тарасов",
            "actors": "Клара Румянова, Анатолий Папанов, Геннадий Хазанов",
            "relevance": 103845624,
            "relevance_prediction": 0.09998173103,
            "provider_info": [
              {
                "type": "tv_show",
                "provider_name": "kinopoisk",
                "provider_item_id": "4ab7044baa2c2dc599c244196429245f",
                "misc_ids": {
                  "kinopoisk": "46483"
                }
              }
            ],
            "debug_info": {
              "web_page_url": "http://www.kinopoisk.ru/film/46483"
            }
          }
        }
      }
    ]
  }
})");

constexpr TStringBuf SHOW_VIDEO_SETTINGS_RESPONSE = TStringBuf(R"(
{
  "blocks": [
    {
      "command_sub_type": "show_video_settings",
      "command_type": "show_video_settings",
      "data": {
          "listening_is_possible": true
      },
      "type": "command"
    },
    {
      "data": {
        "features":{}
      },
      "type": "client_features"
    }
  ],
  "form": {
    "name": "personal_assistant.scenarios.video_command.show_video_settings",
    "slots":[]
  }
}
)");

constexpr TStringBuf EXPECTED_SHOW_VIDEO_SETTINGS_PROTO_ANSWER = TStringBuf(R"(
{
  "analytics_info" : {
    "intent" : "personal_assistant.scenarios.video_command.show_video_settings"
  },
  "layout" : {
    "directives" : [
      {
        "show_video_settings_directive" : {
          "name" : "show_video_settings"
        }
      }
    ],
    "should_listen" : true
  }
}
)");

void RunTest(const TMaybe<TString>& scenario, TStringBuf context, TStringBuf expected) {
    auto ctxPtr = NTestingHelpers::MakeContext(context, false /* shouldValidate */);
    UNIT_ASSERT(ctxPtr);
    TContext& ctx = *ctxPtr;

    auto protoResponseOrError = BassContextToProtocolResponseBody(ctx, scenario);
    auto visitor = MakeLambdaVisitor(
        [&](const NAlice::TError& error) { UNIT_FAIL("Error on making proto response: " << error.ErrorMsg); },
        [&](const TScenarioResponseBody& proto) {
            TString actual = NProtobufJson::Proto2Json(proto, NProtobufJson::TProto2JsonConfig{}.SetUseJsonName(true));
            UNIT_ASSERT(EqualJson(NSc::TValue::FromJson(expected), NSc::TValue::FromJson(actual)));
        });
    Visit(visitor, protoResponseOrError);
}

Y_UNIT_TEST_SUITE(ProtocolScenarioUtils) {
    Y_UNIT_TEST(VideoPlayResponse) {
        RunTest("video_play", VIDEO_PLAY_RESPONSE, EXPECTED_VIDEO_PROTO_ANSWER);
    }

    Y_UNIT_TEST(ShowVideoSettingsResponse) {
        RunTest(Nothing(), SHOW_VIDEO_SETTINGS_RESPONSE, EXPECTED_SHOW_VIDEO_SETTINGS_PROTO_ANSWER);
    }
}

} // namespace
