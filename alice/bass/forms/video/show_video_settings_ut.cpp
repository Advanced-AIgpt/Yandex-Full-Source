#include "change_track.h"

#include <alice/bass/ut/helpers.h>

#include <library/cpp/scheme/scheme.h>

using namespace NTestingHelpers;

namespace {

const NSc::TValue CONTEXT_CHANGE_TRACK = NSc::TValue::FromJson(R"(
{
  "form": {
    "name": "personal_assistant.scenarios.video_command.change_track"
  },
  "meta": {
    "device_state": {
      "video": {
        "current_screen": "video_player",
        "currently_playing": {
          "item": {
            "audio_streams": [
              {
                "title": "Русская озвучка", "language": "rus", "index": 1, "suggest": "Включи русскую озвучку"
              },
              {
                "title": "Английская озвучка", "language": "eng", "index": 2, "suggest": "Включи английскую озвучку"
              },
              {
                "title": "Сыендук 18+", "language": "rus-x-sndk18", "index": 3
              }
            ]
          }
        }
      }
    },
    "device_config": {
      "content_settings": "medium"
    },
    "epoch": 1590389680,
    "tz": "Europe/Moscow",
    "utterance": "покажи настройки",
    "experiments": {
      "show_video_settings": 1
    }
  }
}
)");

Y_UNIT_TEST_SUITE(VideoCommandShowVideoSettingsTestSuite) {

    // Default changing
    Y_UNIT_TEST(ShowVideoSettings) {
        const auto ctx = MakeContext(CONTEXT_CHANGE_TRACK);

        UNIT_ASSERT(!NBASS::NVideo::ShowVideoSettings(*ctx)->GetResult());
        UNIT_ASSERT(ctx->GetCommand("show_video_settings"));
    }
}

} // namespace
