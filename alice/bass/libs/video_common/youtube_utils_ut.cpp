#include "youtube_utils.h"

#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/video_common/video_ut_helpers.h>

#include <alice/library/unittest/ut_helpers.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NVideoCommon;

namespace {

const auto correctItem = NSc::TValue::FromJson(R"(
{
    "id": "id",
    "snippet": {
        "thumbnails": {
            "high": {
                "url": "url"
            },
        },
        "title": "title",
        "publishedAt": "2016-08-08T09:07:58.000Z"
    },
    "contentDetails": {
        "duration": "PT3M29S"
    }
}
)");

const auto itemNoId = NSc::TValue::FromJson(R"(
{
    "snippet": {
        "thumbnails": {
            "high": {
                "url": "url"
            }
        }
        "title": "title"
    }
}
)");

const auto itemNoImg = NSc::TValue::FromJson(R"(
{
    "id": "id",
    "snippet": {
        "title": "title"
    }
}
)");

const auto itemNoTitle = NSc::TValue::FromJson(R"(
{
    "id": "id",
    "snippet": {
        "thumbnails": {
            "high": {
                "url": "url",
            }
        }
    }
}
)");

const auto itemEmpty = NSc::TValue::FromJson("");

Y_UNIT_TEST_SUITE(YouTubeVideoProviderUnitTests) {
    Y_UNIT_TEST(ContentInfoProvider) {
        {
            auto item = TryParseYouTubeNode(correctItem);
            UNIT_ASSERT(item);
            UNIT_ASSERT((*item)->ReleaseYear().Get() == 2016);
            UNIT_ASSERT((*item)->Duration().Get() == 209);
        }
        {
            auto item = TryParseYouTubeNode(itemNoId);
            UNIT_ASSERT(!item);
        }
        {
            auto item = TryParseYouTubeNode(itemNoImg);
            UNIT_ASSERT(!item);
        }
        {
            auto item = TryParseYouTubeNode(itemNoTitle);
            UNIT_ASSERT(!item);
        }
        {
            auto item = TryParseYouTubeNode(itemEmpty);
            UNIT_ASSERT(!item);
        }
    }
}

} // namespace
