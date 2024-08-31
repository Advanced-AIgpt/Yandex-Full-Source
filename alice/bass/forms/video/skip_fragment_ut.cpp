#include "skip_fragment.h"

#include <alice/bass/ut/helpers.h>

#include <library/cpp/scheme/scheme.h>

using namespace NTestingHelpers;

namespace {

const NSc::TValue CONTEXT_SKIP_VIDEO_FRAGMENT_DEPR = NSc::TValue::FromJson(R"(
{
  "form": {
    "name": "personal_assistant.scenarios.video_command.skip_video_fragment"
  },
  "meta": {
    "device_state": {
      "video": {
        "current_screen": "video_player",
        "currently_playing": {
          "item": {
            "skippable_fragments_depr": {
              "credits_start": 1227, "intro_end": 124, "intro_start": 94
            }
          },
          "progress": {
            "duration": 1302,
            "played": 0
          }
        }
      }
    },
    "epoch": 1590389680,
    "tz": "Europe/Moscow",
    "utterance": "перемотай заставку",
    "experiments": {
      "skip_video_fragment": 1
    }
  }
}
)");

const NSc::TValue CONTEXT_SKIP_VIDEO_FRAGMENT = NSc::TValue::FromJson(R"(
{
  "form": {
    "name": "personal_assistant.scenarios.video_command.skip_video_fragment"
  },
  "meta": {
    "device_state": {
      "video": {
        "current_screen": "video_player",
        "currently_playing": {
          "item": {
            "skippable_fragments": [
              {
                "end_time": 1302, "start_time": 1000, "type": "credits"
              },
              {
                "end_time": 150, "start_time": 100, "type": "recap"
              },
              {
                "end_time": 70, "start_time": 10
              }
            ]
          },
          "progress": {
            "duration": 1302,
            "played": 0
          }
        }
      }
    },
    "epoch": 1590389680,
    "tz": "Europe/Moscow",
    "utterance": "перемотай заставку",
    "experiments": {
      "skip_video_fragment": 1
    }
  }
}
)");

NBASS::TContext::TPtr GetSkipVideoFragmentContext(
    const NSc::TValue& videoCtx,
    const ui64 played,
    const bool addFragmentSlot)
{
    NSc::TValue modifiedCtx = videoCtx;
    modifiedCtx["meta"]["device_state"]["video"]["currently_playing"]["progress"]["played"] = played;
    if (addFragmentSlot) {
        NSc::TValue& slots = modifiedCtx["form"]["slots"].SetArray();
        NBASS::TContext::TSlot fragmentSlot("fragment", "string");
        fragmentSlot.Value = "заставку";
        slots.Push(fragmentSlot.ToJson());
    }
    const auto ctx = MakeContext(modifiedCtx);
    UNIT_ASSERT(ctx);
    return ctx;
}

Y_UNIT_TEST_SUITE(VideoCommandSkipVideoFragmentTestSuite) {
    // Deprecated version
    Y_UNIT_TEST(SkipIntroFragmentDepr) {
        const auto ctx = GetSkipVideoFragmentContext(
            CONTEXT_SKIP_VIDEO_FRAGMENT_DEPR /* videoCtx */,
            95 /* played */,
            false /* addFragmentSlot */);

        UNIT_ASSERT(!NBASS::NVideo::SkipFragment(*ctx)->GetResult());
    }

    Y_UNIT_TEST(SkipCreditsFragmentDepr) {
        const auto ctx = GetSkipVideoFragmentContext(
            CONTEXT_SKIP_VIDEO_FRAGMENT_DEPR /* videoCtx */,
            1227 /* played */,
            false /* addFragmentSlot */);

        UNIT_ASSERT(!NBASS::NVideo::SkipFragment(*ctx)->GetResult());
    }

    Y_UNIT_TEST(SkipUnskippableFragmentDepr) {
        const auto ctx = GetSkipVideoFragmentContext(
            CONTEXT_SKIP_VIDEO_FRAGMENT_DEPR /* videoCtx */,
            124 /* played */,
            false /* addFragmentSlot */);

        UNIT_ASSERT(NBASS::NVideo::SkipFragment(*ctx)->GetResult());
    }

    Y_UNIT_TEST(SkipUnskippableFragmentWithSlotDepr) {
        const auto ctx = GetSkipVideoFragmentContext(
            CONTEXT_SKIP_VIDEO_FRAGMENT_DEPR /* videoCtx */,
            124 /* played */,
            true /* addFragmentSlot */);

        UNIT_ASSERT(!NBASS::NVideo::SkipFragment(*ctx)->GetResult());
        UNIT_ASSERT(ctx->HasAttention(NBASS::NVideo::ATTENTION_VIDEO_NOT_SKIPPABLE_FRAGMENT));
    }

    // New version
    Y_UNIT_TEST(SkipRecapFragment) {
        const auto ctx = GetSkipVideoFragmentContext(
            CONTEXT_SKIP_VIDEO_FRAGMENT /* videoCtx */,
            120 /* played */,
            false /* addFragmentSlot */);

        UNIT_ASSERT(!NBASS::NVideo::SkipFragment(*ctx)->GetResult());
    }

    Y_UNIT_TEST(SkipUnskippableFragment) {
        const auto ctx = GetSkipVideoFragmentContext(
            CONTEXT_SKIP_VIDEO_FRAGMENT /* videoCtx */,
            70 /* played */,
            false /* addFragmentSlot */);

        UNIT_ASSERT(NBASS::NVideo::SkipFragment(*ctx)->GetResult());
    }

    Y_UNIT_TEST(SkipUnskippableFragmentWithSlot) {
        const auto ctx = GetSkipVideoFragmentContext(
            CONTEXT_SKIP_VIDEO_FRAGMENT /* videoCtx */,
            70 /* played */,
            true /* addFragmentSlot */);

        UNIT_ASSERT(!NBASS::NVideo::SkipFragment(*ctx)->GetResult());
        UNIT_ASSERT(ctx->HasAttention(NBASS::NVideo::ATTENTION_VIDEO_NOT_SKIPPABLE_FRAGMENT));
    }

    Y_UNIT_TEST(SkipCreditsFragment) {
        const auto ctx = GetSkipVideoFragmentContext(
            CONTEXT_SKIP_VIDEO_FRAGMENT /* videoCtx */,
            1100 /* played */,
            false /* addFragmentSlot */);

        UNIT_ASSERT(!NBASS::NVideo::SkipFragment(*ctx)->GetResult());
    }
}

} // namespace
