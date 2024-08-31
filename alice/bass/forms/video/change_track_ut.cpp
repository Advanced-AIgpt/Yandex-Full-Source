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
            ],
            "subtitles": [
              {
                "title": "Субтитры Сыендука", "language": "rus-x-sndk", "index": 4, "suggest": "Включи русские субтитры"
              },
              {
                "title": "Английские субтитры", "language" : "eng", "index": 5, "suggest": "Включи английские субтитры"
              },
              {
                "title": "Сыендук 18+", "language": "rus-x-18", "index": 6, "suggest": "Включи русские субтитры 18+"
              }
            ],
            "provider_name" : "kinopoisk"
          },
          "audio_language" : "rus-x-sndk18",
          "subtitles_language" : "eng",
          "subtitles_button_enable" : true
        }
      }
    },
    "epoch": 1590389680,
    "tz": "Europe/Moscow",
    "utterance": "",
    "experiments": {
      "change_track": 1,
      "show_video_settings": 1
    }
  }
}
)");

const NSc::TValue CONTEXT_CHANGE_TRACK_WITHOUT_SUBTILES = NSc::TValue::FromJson(R"(
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
              }
            ],
            "subtitles": [
            ],
            "provider_name" : "kinopoisk"
          },
          "audio_language" : "rus",
          "subtitles_button_enable" : true
        }
      }
    },
    "epoch": 1590389680,
    "tz": "Europe/Moscow",
    "utterance": "",
    "experiments": {
      "change_track": 1,
      "show_video_settings": 1
    }
  }
}
)");

const NSc::TValue CONTEXT_IRRELEVANT_PROVIDER = NSc::TValue::FromJson(R"(
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
            ],
            "subtitles": [
            ],
            "provider_name" : "ivi"
          }
        }
      }
    },
    "epoch": 1590389680,
    "tz": "Europe/Moscow",
    "utterance": "",
    "experiments": {
      "change_track": 1,
      "show_video_settings": 1
    }
  }
}
)");

const NSc::TValue CONTEXT_SUBTITLES_BUTTON_ENABLE_FALSE = NSc::TValue::FromJson(R"(
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
                "title": "Английская озвучка", "language": "eng", "index": 1, "suggest": "Включи английскую озвучку"
              }
            ],
            "subtitles": [
              {
                "title": "Выключены", "language": "off", "index": 2, "suggest": "Выключи субтитры"
              },
              {
                "title": "Английские субтитры", "language" : "eng", "index": 3, "suggest": "Включи английские субтитры"
              }
            ],
            "provider_name" : "kinopoisk"
          },
          "audio_language" : "eng",
          "subtitles_language" : "rus",
          "subtitles_button_enable" : false
        }
      }
    },
    "epoch": 1590389680,
    "tz": "Europe/Moscow",
    "utterance": "",
    "experiments": {
      "change_track": 1,
      "show_video_settings": 1
    }
  }
}
)");

NBASS::TContext::TPtr GetChangeTrackContext(
    const NSc::TValue& videoCtx,
    const THashMap<TStringBuf, TStringBuf>& slotsInfo)
{
    NSc::TValue modifiedCtx = videoCtx;
    if (slotsInfo) {
        NSc::TValue& slots = modifiedCtx["form"]["slots"].SetArray();
        for (const auto& [slotName, slotValue] : slotsInfo) {
            NBASS::TContext::TSlot slot(slotName, "string");
            slot.Value = slotValue;
            slots.Push(slot.ToJson());
        }
    }
    const auto ctx = MakeContext(modifiedCtx);
    UNIT_ASSERT(ctx);
    return ctx;
}

Y_UNIT_TEST_SUITE(VideoCommandChangeTrackTestSuite) {

    // Default changing
    Y_UNIT_TEST(ChangeAudioTrack) {
        const THashMap<TStringBuf, TStringBuf> slots = {
            {"audio", "rus"}
        };

        const auto ctx = GetChangeTrackContext(
            CONTEXT_CHANGE_TRACK,
            slots);

        UNIT_ASSERT(!NBASS::NVideo::ChangeTrack(*ctx)->GetResult());
        UNIT_ASSERT(ctx->GetCommand("change_audio"));
    }

    Y_UNIT_TEST(ChangeAudioTrackIrrelevant) {
        const THashMap<TStringBuf, TStringBuf> slots = {
            {"audio", "fra"}
        };

        const auto ctx = GetChangeTrackContext(
            CONTEXT_CHANGE_TRACK,
            slots);

        UNIT_ASSERT(!NBASS::NVideo::ChangeTrack(*ctx)->GetResult());
        UNIT_ASSERT(!ctx->GetCommand("change_audio"));
        UNIT_ASSERT(ctx->HasAttention(NBASS::NVideo::ATTENTION_VIDEO_NO_SUCH_AUDIO_STREAM));
    }

    Y_UNIT_TEST(ChangeSubtitleTrack) {
        const THashMap<TStringBuf, TStringBuf> slots = {
            {"subtitles", "rus"}
        };

        const auto ctx = GetChangeTrackContext(
            CONTEXT_CHANGE_TRACK,
            slots);

        UNIT_ASSERT(!NBASS::NVideo::ChangeTrack(*ctx)->GetResult());
        UNIT_ASSERT(ctx->GetCommand("change_subtitles"));
    }

    Y_UNIT_TEST(ChangeSubtitleTrackIrrelevant) {
        const THashMap<TStringBuf, TStringBuf> slots = {
            {"subtitles", "rus-x-kubik"}
        };

        const auto ctx = GetChangeTrackContext(
            CONTEXT_CHANGE_TRACK,
            slots);

        UNIT_ASSERT(!NBASS::NVideo::ChangeTrack(*ctx)->GetResult());
        UNIT_ASSERT(!ctx->GetCommand("change_audio"));
        UNIT_ASSERT(ctx->GetCommand("show_video_settings"));
        UNIT_ASSERT(ctx->HasAttention(NBASS::NVideo::ATTENTION_VIDEO_NO_SUCH_SUBTITLE));
        UNIT_ASSERT(ctx->HasAttention(NBASS::NVideo::ATTENTION_VIDEO_HAS_SIMILAR_SUBTITLES));
    }

    Y_UNIT_TEST(ChangeBothTrackTypes) {
        const THashMap<TStringBuf, TStringBuf> slots = {
            {"audio", "eng"},
            {"subtitles", "rus-x-sndk"}
        };

        const auto ctx = GetChangeTrackContext(
            CONTEXT_CHANGE_TRACK,
            slots);

        UNIT_ASSERT(!NBASS::NVideo::ChangeTrack(*ctx)->GetResult());
        UNIT_ASSERT(ctx->GetCommand("change_audio"));
        UNIT_ASSERT(ctx->GetCommand("change_subtitles"));
        UNIT_ASSERT(ctx->HasAttention(NBASS::NVideo::ATTENTION_VIDEO_BOTH_TRACK_TYPES));
    }

    Y_UNIT_TEST(ChangeBothTrackTypesIrrelevant) {
        const THashMap<TStringBuf, TStringBuf> slots = {
            {"audio", "fra"},
            {"subtitles", "rus-x-sndk"}
        };

        const auto ctx = GetChangeTrackContext(
            CONTEXT_CHANGE_TRACK,
            slots);

        UNIT_ASSERT(!NBASS::NVideo::ChangeTrack(*ctx)->GetResult());
        UNIT_ASSERT(!ctx->GetCommand("change_audio"));
        UNIT_ASSERT(!ctx->GetCommand("change_subtitles"));
        UNIT_ASSERT(ctx->GetCommand("show_video_settings"));
        UNIT_ASSERT(ctx->HasAttention(NBASS::NVideo::ATTENTION_VIDEO_NO_SUCH_AUDIO_STREAM));
        UNIT_ASSERT(ctx->HasAttention(NBASS::NVideo::ATTENTION_VIDEO_BOTH_TRACK_TYPES));
    }

    Y_UNIT_TEST(ChangeBothTrackTypesWithAny) {
        const THashMap<TStringBuf, TStringBuf> slots = {
            {"audio", "eng"},
            {"subtitles", "any"}
        };

        const auto ctx = GetChangeTrackContext(
            CONTEXT_CHANGE_TRACK,
            slots);

        UNIT_ASSERT(!NBASS::NVideo::ChangeTrack(*ctx)->GetResult());
        UNIT_ASSERT(ctx->GetCommand("change_audio"));
        UNIT_ASSERT(ctx->GetCommand("change_subtitles"));
        UNIT_ASSERT(!ctx->GetCommand("show_video_settings"));
    }

    Y_UNIT_TEST(ChangeBothTrackTypesWithAnyWithouthSubtitles) {
        const THashMap<TStringBuf, TStringBuf> slots = {
            {"audio", "rus"},
            {"subtitles", "any"}
        };

        const auto ctx = GetChangeTrackContext(
            CONTEXT_CHANGE_TRACK_WITHOUT_SUBTILES,
            slots);

        UNIT_ASSERT(!NBASS::NVideo::ChangeTrack(*ctx)->GetResult());
        UNIT_ASSERT(!ctx->GetCommand("change_audio"));
        UNIT_ASSERT(!ctx->GetCommand("change_subtitles"));
        UNIT_ASSERT(ctx->GetCommand("show_video_settings"));
        UNIT_ASSERT(ctx->HasAttention(NBASS::NVideo::ATTENTION_VIDEO_NO_ANY_SUBTITLES));
        UNIT_ASSERT(ctx->HasAttention(NBASS::NVideo::ATTENTION_VIDEO_BOTH_TRACK_TYPES));
    }

    // Change track types using numbers
    Y_UNIT_TEST(ChangeAudioTrackWithNumber) {
        const THashMap<TStringBuf, TStringBuf> slots = {
            {"firstNumber", "1"}
        };

        const auto ctx = GetChangeTrackContext(
            CONTEXT_CHANGE_TRACK,
            slots);

        UNIT_ASSERT(!NBASS::NVideo::ChangeTrack(*ctx)->GetResult());
        UNIT_ASSERT(ctx->GetCommand("change_audio"));
    }

    Y_UNIT_TEST(ChangeSubtitleTrackWithNumber) {
        const THashMap<TStringBuf, TStringBuf> slots = {
            {"firstNumber", "4"}
        };

        const auto ctx = GetChangeTrackContext(
            CONTEXT_CHANGE_TRACK,
            slots);

        UNIT_ASSERT(!NBASS::NVideo::ChangeTrack(*ctx)->GetResult());
        UNIT_ASSERT(ctx->GetCommand("change_subtitles"));
    }

    Y_UNIT_TEST(ChangeBothTrackTypesWithNumber) {
        const THashMap<TStringBuf, TStringBuf> slots = {
            {"firstNumber", "1"},
            {"secondNumber", "4"}
        };

        const auto ctx = GetChangeTrackContext(
            CONTEXT_CHANGE_TRACK,
            slots);

        UNIT_ASSERT(!NBASS::NVideo::ChangeTrack(*ctx)->GetResult());
        UNIT_ASSERT(ctx->GetCommand("change_audio"));
        UNIT_ASSERT(ctx->GetCommand("change_subtitles"));
    }

    Y_UNIT_TEST(ChangeBothTrackTypesWithNumberIrrelevant) {
        const THashMap<TStringBuf, TStringBuf> slots = {
            {"firstNumber", "1"},
            {"secondNumber", "7"}
        };

        const auto ctx = GetChangeTrackContext(
            CONTEXT_CHANGE_TRACK,
            slots);

        UNIT_ASSERT(!NBASS::NVideo::ChangeTrack(*ctx)->GetResult());
        UNIT_ASSERT(!ctx->GetCommand("change_audio"));
        UNIT_ASSERT(!ctx->GetCommand("change_subtitles"));
        UNIT_ASSERT(ctx->HasAttention(NBASS::NVideo::ATTENTION_VIDEO_IRRELEVANT_NUMBER));
    }

    Y_UNIT_TEST(ChangeBothTrackTypesWithNumberError) {
        const THashMap<TStringBuf, TStringBuf> slots = {
            {"firstNumber", "1"},
            {"secondNumber", "10"}
        };

        const auto ctx = GetChangeTrackContext(
            CONTEXT_CHANGE_TRACK,
            slots);

        UNIT_ASSERT(NBASS::NVideo::ChangeTrack(*ctx)->GetResult());
    }

    // Change track type to Lang18plus
    Y_UNIT_TEST(ChangeLang18plusAudioTrack) {
        const THashMap<TStringBuf, TStringBuf> slots = {
            {"audio", "18plus"},
        };

        const auto ctx = GetChangeTrackContext(
            CONTEXT_CHANGE_TRACK,
            slots);

        UNIT_ASSERT(!NBASS::NVideo::ChangeTrack(*ctx)->GetResult());
        UNIT_ASSERT(ctx->GetCommand("change_audio"));
    }

    Y_UNIT_TEST(ChangeLang18plusSubtitleTrack) {
        const THashMap<TStringBuf, TStringBuf> slots = {
            {"subtitles", "18plus"}
        };

        const auto ctx = GetChangeTrackContext(
            CONTEXT_CHANGE_TRACK,
            slots);

        UNIT_ASSERT(!NBASS::NVideo::ChangeTrack(*ctx)->GetResult());
        UNIT_ASSERT(ctx->GetCommand("change_subtitles"));
    }

    // Change subtitles with flag SubtitlesButtonEnable = false
    Y_UNIT_TEST(ChangeSubtitleTrackTurnOff) {
        const THashMap<TStringBuf, TStringBuf> slots = {
            {"subtitles", "off"}
        };

        const auto ctx = GetChangeTrackContext(
            CONTEXT_SUBTITLES_BUTTON_ENABLE_FALSE,
            slots);

        UNIT_ASSERT(!NBASS::NVideo::ChangeTrack(*ctx)->GetResult());
        UNIT_ASSERT(!ctx->GetCommand("change_subtitles"));
        UNIT_ASSERT(ctx->HasAttention(NBASS::NVideo::ATTENTION_VIDEO_CANNOT_TURN_OFF_SUBTITLES));
    }

    Y_UNIT_TEST(ChangeSubtitleTrackTurnOffWithNumber) {
        const THashMap<TStringBuf, TStringBuf> slots = {
            {"firstNumber", "2"}
        };

        const auto ctx = GetChangeTrackContext(
            CONTEXT_SUBTITLES_BUTTON_ENABLE_FALSE,
            slots);

        UNIT_ASSERT(!NBASS::NVideo::ChangeTrack(*ctx)->GetResult());
        UNIT_ASSERT(!ctx->GetCommand("change_subtitles"));
        UNIT_ASSERT(ctx->HasAttention(NBASS::NVideo::ATTENTION_VIDEO_CANNOT_TURN_OFF_SUBTITLES));
    }

    Y_UNIT_TEST(ChangeSubtitleTrackWithButtonEnableFlag) {
        const THashMap<TStringBuf, TStringBuf> slots = {
            {"subtitles", "eng"}
        };

        const auto ctx = GetChangeTrackContext(
            CONTEXT_SUBTITLES_BUTTON_ENABLE_FALSE,
            slots);

        UNIT_ASSERT(!NBASS::NVideo::ChangeTrack(*ctx)->GetResult());
        UNIT_ASSERT(ctx->GetCommand("change_subtitles"));
    }

    Y_UNIT_TEST(ChangeTrackIrrelevantProvider) {
        const THashMap<TStringBuf, TStringBuf> slots = {
            {"audio", "eng"}
        };

        const auto ctx = GetChangeTrackContext(
            CONTEXT_IRRELEVANT_PROVIDER,
            slots);

        UNIT_ASSERT(!NBASS::NVideo::ChangeTrack(*ctx)->GetResult());
        UNIT_ASSERT(ctx->HasAttention(NBASS::NVideo::ATTENTION_VIDEO_IRRELEVANT_PROVIDER));
    }

}

} // namespace
