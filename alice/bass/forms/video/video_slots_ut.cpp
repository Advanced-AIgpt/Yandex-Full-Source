#include "video_slots.h"

#include <alice/bass/ut/helpers.h>

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice::NVideoCommon;
using namespace NBASS;
using namespace NBASS::NVideo;
using namespace NTestingHelpers;

namespace {
// epoch == 1495574317 is 2017 year
const auto IVI_REQUEST = NSc::TValue::FromJson(R"({
  "form": {
    "name": "personal_assistant.scenarios.video_play",
    "slots": [
      {"name": "search_text", "optional": true, "source_text": "терминатор", "type": "string", "value": "терминатор"},
      {"name": "action", "optional": true, "source_text": "включи", "type": "video_action", "value": "play"},
      {"name": "content_provider", "optional": true, "source_text": "ivi", "type": "video_provider", "value": "ivi"},
      {"name": "release_date", "optional": true, "type": "year_adjective", "value": "-1"}
    ]
  },
  "meta": {
    "epoch": 1495574317,
    "tz": "Europe/Moscow",
    "utterance": "включи прошлогоднего терминатор на ivi"
  }
}
)");

const auto IVI_REQUEST_INVALID_TZ = NSc::TValue::FromJson(R"({
  "form": {
    "name": "personal_assistant.scenarios.video_play",
    "slots": [
      {"name": "search_text", "optional": true, "source_text": "терминатор", "type": "string", "value": "терминатор"},
      {"name": "action", "optional": true, "source_text": "включи", "type": "video_action", "value": "play"},
      {"name": "content_provider", "optional": true, "source_text": "ivi", "type": "video_provider", "value": "ivi"},
      {"name": "release_date", "optional": true, "type": "year_adjective", "value": "-1"}
    ]
  },
  "meta": {
    "epoch": 1495574317,
    "tz": "InvalidTZ",
    "utterance": "включи прошлогоднего терминатор на ivi"
  }
}
)");

const auto IVI_REQUEST_ABSOLUTE_RELEASE_DATE = NSc::TValue::FromJson(R"({
  "form": {
    "name": "personal_assistant.scenarios.video_play",
    "slots": [
      {"name": "search_text", "optional": true, "source_text": "терминатор", "type": "string", "value": "терминатор"},
      {"name": "action", "optional": true, "source_text": "включи", "type": "video_action", "value": "play"},
      {"name": "content_provider", "optional": true, "source_text": "ivi", "type": "video_provider", "value": "ivi"},
      {"name": "release_date", "optional": true, "type": "date", "value": "2016"}
    ]
  },
  "meta": {
    "epoch": 1495574317,
    "tz": "InvalidTZ",
    "utterance": "включи прошлогоднего терминатор на ivi"
  }
}
)");

const auto HDREZKA_REQUEST = NSc::TValue::FromJson(R"({
  "form": {
    "name": "personal_assistant.scenarios.video_play",
    "slots": [
      {"name": "search_text", "optional": true, "source_text": "терминатор", "type": "string", "value": "терминатор"},
      {"name": "action", "optional": true, "source_text": "включи", "type": "video_action", "value": "play"},
      {"name": "content_provider", "optional": true, "source_text": "хдрезке", "type": "string", "value": "хдрезке"},
      {"name": "release_date", "optional": true, "type": "year_adjective", "value": "2000:2009"}
    ]
  },
  "meta": {
    "epoch": 1495574317,
    "tz": "Europe/Moscow",
    "utterance": "включи терминатор на хдрезке"
  }
}
)");

const auto NO_PROVIDER_REQUEST = NSc::TValue::FromJson(R"(
{
  "form": {
    "name": "personal_assistant.scenarios.video_play",
    "slots": [{
      "name": "search_text",
      "optional": true,
      "source_text": "3 богатыря и морской царь",
      "type": "string",
      "value": "3 богатыря и морской царь"
     }]
  },
  "meta" : {
    "epoch": 1495574317,
    "tz": "Europe/Moscow",
    "utterance": "три богатыря и морской царь"
   }
}
)");

const auto SEARCH_REQUEST_WITH_CONTENT_TYPE = NSc::TValue::FromJson(R"(
{
  "form": {
    "name": "personal_assistant.scenarios.video_play",
    "slots": [
      { "name": "search_text","optional": true, "source_text": "терминатор", "type": "string", "value": "терминатор" },
      { "name": "action", "optional": true, "source_text": "найди", "type": "video_action", "value": "find" },
      { "name": "content_provider", "optional": true, "source_text": "амедиатека", "type": "video_provider", "value": "amediateka" },
      { "name": "content_type", "optional": true, "source_text": "фильм", "type": "video_content_type", "value": "movie" }
    ]
  },
  "meta": {
    "device_state": {
      "is_tv_plugged_in": true
    },
    "epoch": 1495574317,
    "tz": "Europe/Moscow",
    "utterance": "найди фильм терминатор на амедиатека"
  }
}
)");

Y_UNIT_TEST_SUITE(VideoSlotsTestSuite) {
    Y_UNIT_TEST(ProviderOverrideWithKp) {
        const auto ctx = MakeContext(IVI_REQUEST);
        const TMaybe<TVideoSlots> slots = TVideoSlots::TryGetFromContext(*ctx);
        UNIT_ASSERT(slots);
        const auto& original = slots->OriginalProvider;
        UNIT_ASSERT(original.Defined());
        UNIT_ASSERT_STRINGS_EQUAL(original.GetString(), PROVIDER_KINOPOISK);
        UNIT_ASSERT_STRINGS_EQUAL(original.GetType(), SLOT_PROVIDER_TYPE);
        UNIT_ASSERT_STRINGS_EQUAL(slots->FixedProvider, PROVIDER_KINOPOISK);
        UNIT_ASSERT(!slots->ProviderWasChanged);

        UNIT_ASSERT_VALUES_EQUAL(slots->ReleaseDate.ExactYear, 2016u);
        UNIT_ASSERT_VALUES_EQUAL(slots->ReleaseDate.RelativeYear, -1);
    }

    Y_UNIT_TEST(KeepProvider) {
        auto request = IVI_REQUEST;
        request["meta"]["experiments"][NAlice::NVideoCommon::FLAG_VIDEO_UNBAN_IVI].SetBool(true);
        const auto ctx = MakeContext(request);
        const TMaybe<TVideoSlots> slots = TVideoSlots::TryGetFromContext(*ctx);
        UNIT_ASSERT(slots);
        const auto& original = slots->OriginalProvider;
        UNIT_ASSERT(original.Defined());
        UNIT_ASSERT_STRINGS_EQUAL(original.GetString(), PROVIDER_IVI);
        UNIT_ASSERT_STRINGS_EQUAL(original.GetType(), SLOT_PROVIDER_TYPE);
        UNIT_ASSERT_STRINGS_EQUAL(slots->FixedProvider, PROVIDER_IVI);
        UNIT_ASSERT(!slots->ProviderWasChanged);
    }

    Y_UNIT_TEST(ChangeProvider) {
        const auto ctx = MakeContext(HDREZKA_REQUEST);
        const TMaybe<TVideoSlots> slots = TVideoSlots::TryGetFromContext(*ctx);
        UNIT_ASSERT(slots);
        const auto& original = slots->OriginalProvider;
        UNIT_ASSERT(original.Defined());
        UNIT_ASSERT_STRINGS_EQUAL(original.GetString(), "хдрезке");
        UNIT_ASSERT_STRINGS_EQUAL(original.GetType(), "string");
        UNIT_ASSERT_STRINGS_EQUAL(slots->FixedProvider, "youtube");
        UNIT_ASSERT(slots->ProviderWasChanged);

        UNIT_ASSERT_VALUES_EQUAL(slots->ReleaseDate.DecadeStartYear, 2000u);
    }

    Y_UNIT_TEST(NoProvider) {
        const auto ctx = MakeContext(NO_PROVIDER_REQUEST);
        const TMaybe<TVideoSlots> slots = TVideoSlots::TryGetFromContext(*ctx);
        UNIT_ASSERT(slots);
        const auto& original = slots->OriginalProvider;
        UNIT_ASSERT(!original.Defined());
        UNIT_ASSERT_STRINGS_EQUAL(slots->FixedProvider, "");
        UNIT_ASSERT(!slots->ProviderWasChanged);
    }

    Y_UNIT_TEST(BuildSearchQueryForWeb) {
        {
            const auto ctx = MakeContext(SEARCH_REQUEST_WITH_CONTENT_TYPE);
            const TMaybe<TVideoSlots> slots = TVideoSlots::TryGetFromContext(*ctx);
            UNIT_ASSERT(slots);
            TString webQuery = slots->BuildSearchQueryForWeb();
            UNIT_ASSERT_VALUES_EQUAL(webQuery, "фильм терминатор");
        }
        {
            NSc::TValue json = SEARCH_REQUEST_WITH_CONTENT_TYPE;
            json["meta"]["experiments"][FLAG_VIDEO_DONT_USE_CONTENT_TYPE_FOR_PROVIDER_SEARCH].SetBool(true);
            const auto ctx = MakeContext(json);
            const TMaybe<TVideoSlots> slots = TVideoSlots::TryGetFromContext(*ctx);
            UNIT_ASSERT(slots);
            TString webQuery = slots->BuildSearchQueryForWeb();
            UNIT_ASSERT_VALUES_EQUAL(webQuery, "терминатор");
        }
    }

    Y_UNIT_TEST(BuildSearchQueryForInternetVideos) {
        {
            const auto ctx = MakeContext(SEARCH_REQUEST_WITH_CONTENT_TYPE);
            const TMaybe<TVideoSlots> slots = TVideoSlots::TryGetFromContext(*ctx);
            UNIT_ASSERT(slots);
            TString webQuery = slots->BuildSearchQueryForInternetVideos();
            UNIT_ASSERT_VALUES_EQUAL(webQuery, "фильм терминатор");
        }
        {
            // For video search, the result should not depend on flags.
            NSc::TValue json = SEARCH_REQUEST_WITH_CONTENT_TYPE;
            json["meta"]["experiments"][FLAG_VIDEO_DONT_USE_CONTENT_TYPE_FOR_PROVIDER_SEARCH].SetBool(true);
            const auto ctx = MakeContext(json);
            const TMaybe<TVideoSlots> slots = TVideoSlots::TryGetFromContext(*ctx);
            UNIT_ASSERT(slots);
            TString webQuery = slots->BuildSearchQueryForInternetVideos();
            UNIT_ASSERT_VALUES_EQUAL(webQuery, "фильм терминатор");
        }
    }

    Y_UNIT_TEST(GetExactYear) {
        for (const auto& request : {IVI_REQUEST, IVI_REQUEST_ABSOLUTE_RELEASE_DATE}) {
            const auto ctx = MakeContext(request);
            const TMaybe<TVideoSlots> slots = TVideoSlots::TryGetFromContext(*ctx);
            UNIT_ASSERT(slots);

            UNIT_ASSERT_VALUES_EQUAL(slots->ReleaseDate.ExactYear, 2016u);
        }
    }

    Y_UNIT_TEST(GetExactYearInvalidTz) {
          const auto ctx = MakeContext(IVI_REQUEST_INVALID_TZ);
          const TMaybe<TVideoSlots> slots = TVideoSlots::TryGetFromContext(*ctx);
          UNIT_ASSERT(slots);
          UNIT_ASSERT(!slots->ReleaseDate.ExactYear);
          UNIT_ASSERT_VALUES_EQUAL(slots->ReleaseDate.RelativeYear, -1);
    }
}
} // namespace
