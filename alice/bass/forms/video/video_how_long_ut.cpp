#include "video_how_long.h"

#include <alice/bass/ut/helpers.h>

#include <library/cpp/scheme/scheme.h>

using namespace NTestingHelpers;

namespace {

const NSc::TValue CONTEXT_CHANGE_TRACK = NSc::TValue::FromJson(R"(
{
  "form": {
    "name": "personal_assistant.scenarios.video_command.video_how_long"
  },
  "meta": {
    "device_state": {
      "video": {
        "current_screen": "video_player",
        "currently_playing": {
          "item": {
            "skippable_fragments": [
              {
                "end_time": 999, "start_time": 900, "type": "credits"
              },
              {
                "end_time": 150, "start_time": 100, "type": "recap"
              },
              {
                "end_time": 70, "start_time": 10
              }
            ],
            "type": "",
            "genre": ""
          },
          "progress": {
          }
        }
      }
    },
    "epoch": 1590389680,
    "tz": "Europe/Moscow",
    "utterance": "сколько осталось",
    "experiments": {
      "video_how_long": 1
    }
  }
}
)");

NSc::TValue GetContextWithChangedTypeGenreProgressDuration(
    NSc::TValue context,
    TStringBuf type,
    TStringBuf genre,
    double played,
    TMaybe<double> duration = Nothing())
{
    context["meta"]["device_state"]["video"]["currently_playing"]["item"]["type"].SetString(type);
    context["meta"]["device_state"]["video"]["currently_playing"]["item"]["genre"].SetString(genre);
    context["meta"]["device_state"]["video"]["currently_playing"]["progress"]["played"].SetNumber(played);
    if (duration.Defined()) {
        context["meta"]["device_state"]["video"]["currently_playing"]["progress"]["duration"].SetNumber(duration.GetRef());
    }
    return context;
}

Y_UNIT_TEST_SUITE(VideoCommandVideoHowLongTestSuite) {

    Y_UNIT_TEST(VideoHowLongOnlyMinutes) {
        const auto ctx = MakeContext(
            GetContextWithChangedTypeGenreProgressDuration(CONTEXT_CHANGE_TRACK, "movie", "мультфильм", 7000 /* played */, 7200 /* duration */));

        UNIT_ASSERT(!NBASS::NVideo::VideoHowLong(*ctx)->GetResult());
        UNIT_ASSERT(ctx->HasAttention("cartoon"));
        UNIT_ASSERT(ctx->GetAttention("cartoon")->TrySelect("data/hours").GetIntNumber() == 0);
        UNIT_ASSERT(ctx->GetAttention("cartoon")->TrySelect("data/minutes").GetIntNumber() == 3);
    }

    Y_UNIT_TEST(VideoHowLongOnlyHours) {
        const auto ctx = MakeContext(
            GetContextWithChangedTypeGenreProgressDuration(CONTEXT_CHANGE_TRACK, "movie", "комедия", 3541 /* played */, 7200 /* duration */));

        UNIT_ASSERT(!NBASS::NVideo::VideoHowLong(*ctx)->GetResult());
        UNIT_ASSERT(ctx->HasAttention("movie"));
        UNIT_ASSERT(ctx->GetAttention("movie")->TrySelect("data/hours").GetIntNumber() == 1);
        UNIT_ASSERT(ctx->GetAttention("movie")->TrySelect("data/minutes").GetIntNumber() == 0);
    }

    Y_UNIT_TEST(VideoHowLongBoth) {
        const auto ctx = MakeContext(
            GetContextWithChangedTypeGenreProgressDuration(CONTEXT_CHANGE_TRACK, "tv_show_episode", "комедия", 1234 /* played */, 7200 /* duration */));

        UNIT_ASSERT(!NBASS::NVideo::VideoHowLong(*ctx)->GetResult());
        UNIT_ASSERT(ctx->HasAttention("tv_show_episode"));
        UNIT_ASSERT(ctx->GetAttention("tv_show_episode")->TrySelect("data/hours").GetIntNumber() == 1);
        UNIT_ASSERT(ctx->GetAttention("tv_show_episode")->TrySelect("data/minutes").GetIntNumber() == 39);
    }

    Y_UNIT_TEST(VideoHowLongTvStream) {
        const auto ctx = MakeContext(
            GetContextWithChangedTypeGenreProgressDuration(CONTEXT_CHANGE_TRACK, "tv_stream", "комедия", 1234 /* played */, 7200 /* duration */));

        UNIT_ASSERT(NBASS::NVideo::VideoHowLong(*ctx)->GetResult());
    }

    Y_UNIT_TEST(VideoHowLongLessThanMinute) {
        const auto ctx = MakeContext(
            GetContextWithChangedTypeGenreProgressDuration(CONTEXT_CHANGE_TRACK, "video", "комедия", 7190 /* played */, 7200 /* duration */));

        UNIT_ASSERT(!NBASS::NVideo::VideoHowLong(*ctx)->GetResult());
        UNIT_ASSERT(ctx->HasAttention("video"));
        UNIT_ASSERT(ctx->GetAttention("video")->TrySelect("data/hours").GetIntNumber() == 0);
        UNIT_ASSERT(ctx->GetAttention("video")->TrySelect("data/minutes").GetIntNumber() == 0);
    }

    Y_UNIT_TEST(VideoHowLongLessThanMinuteWithBadProgressInformation) {
        const auto ctx = MakeContext(
            GetContextWithChangedTypeGenreProgressDuration(CONTEXT_CHANGE_TRACK, "video", "комедия", 7300 /* played */, 7200 /* duration */));

        UNIT_ASSERT(!NBASS::NVideo::VideoHowLong(*ctx)->GetResult());
        UNIT_ASSERT(ctx->HasAttention("video"));
        UNIT_ASSERT(ctx->GetAttention("video")->TrySelect("data/hours").GetIntNumber() == 0);
        UNIT_ASSERT(ctx->GetAttention("video")->TrySelect("data/minutes").GetIntNumber() == 0);
    }


    Y_UNIT_TEST(VideoHowLongDurationNotDefined) {
        const auto ctx = MakeContext(
            GetContextWithChangedTypeGenreProgressDuration(CONTEXT_CHANGE_TRACK, "video", "комедия", 7190 /* played */, Nothing() /* duration */));

        UNIT_ASSERT(NBASS::NVideo::VideoHowLong(*ctx)->GetResult());
    }

    Y_UNIT_TEST(VideoHowLongDurationChangedWithSkippableFragment) {
        const auto ctx = MakeContext(
            GetContextWithChangedTypeGenreProgressDuration(CONTEXT_CHANGE_TRACK, "tv_show_episode", "комедия", 0 /* played */, 1000 /* duration */));

        UNIT_ASSERT(!NBASS::NVideo::VideoHowLong(*ctx)->GetResult());
        UNIT_ASSERT(ctx->HasAttention("has_credits"));
        UNIT_ASSERT(ctx->GetAttention("has_credits")->TrySelect("data/hours").GetIntNumber() == 0);
        UNIT_ASSERT(ctx->GetAttention("has_credits")->TrySelect("data/minutes").GetIntNumber() == 900 / 60);
    }

}

} // namespace
