#include "kinopoisk_provider.h"

#include <alice/bass/ut/helpers.h>

#include <alice/library/video_common/audio_and_subtitle_helper.h>

#include <library/cpp/testing/unittest/registar.h>


using namespace NBASS::NVideo;
using namespace NBASS;
using namespace NHttpFetcher;
using namespace NTestingHelpers;
using namespace NVideoCommon;

namespace {

using TPlayError = IVideoClipsProvider::TPlayError;
using TPlayResult = IVideoClipsProvider::TPlayResult;

constexpr auto APPROVED_EMPTY_DATA = TStringBuf(R"({
    "status": "APPROVED"
})");

constexpr auto APPROVED_DATA = TStringBuf(R"({
    "status": "APPROVED",
    "masterPlaylist": {
        "uuid": "4c29f9787a07c733b63a10bc31146625",
        "uri": "https://strm.yandex.ru/vh-ott-converted/vod-content/282953532/4c29f978-7a07-c733-b63a-10bc31146625.ism/manifest.hd.mpd?ottsession=33de11c7d062411da3d9e76295a22e8a"
    }
})");

constexpr auto REJECTED_EMPTY_DATA = TStringBuf(R"({
    "status": "REJECTED"
})");

constexpr auto REJECTED_DATA = TStringBuf(R"({
    "status": "REJECTED",
    "watchingRejectionReason": "GEO_CONSTRAINT_VIOLATION"
})");

const NSc::TValue PAYLOAD_ALL_STREAMS = NSc::TValue::FromJson(R"(
{
  "allStreams": [
    {
      "audio": [
        {
          "index": 0,
          "isDefault": false,
          "language": "rus-x-sndk",
          "title": "Русский (Сыендук) "
        },
        {
          "index": 1,
          "isDefault": false,
          "language": "eng",
          "title": "Английский"
        }
      ],
      "drmConfig": {
      },
      "drmType": "widevine",
      "streamType": "DASH",
      "subtitles": [
        {
          "isDefault": false,
          "language": "eng",
          "title": "английские"
        },
        {
          "isDefault": false,
          "language": "rus",
          "title": "русские"
        }
      ]
    }
  ],
  "streams": {
    "audio": [
      {
        "index": 0,
        "isDefault": false,
        "language": "rus-x-sndk",
        "title": "Русский2 (Сыендук) "
      },
      {
        "index": 1,
        "isDefault": false,
        "language": "eng",
        "title": "Английский"
      }
    ],
    "drmConfig": {
    },
    "subtitles": [
      {
        "isDefault": false,
        "language": "eng",
        "title": "английские"
      },
      {
        "isDefault": false,
        "language": "rus",
        "title": "русские"
      }
    ]
  }
}
)");

const NSc::TValue SKIPPABLE_FRAGMENTS_NOT_SORTED = NSc::TValue::FromJson(R"(
[
  {"endTime":149,"startTime":118,"type":"credits"},
  {"endTime":1322,"startTime":1313,"type":"credits"},
  {"endTime":148,"startTime":118,"type":"intro"}
]
)");

TPlayResult ParseMasterPlaylistResponse(TStringBuf data, TPlayVideoCommandData& commandData) {
    return TKinopoiskClipsProvider::ParseMasterPlaylistResponse(data, commandData.Scheme());
}

Y_UNIT_TEST_SUITE_F(KinopoiskProviderUnitTests, TBassContextFixture) {
    Y_UNIT_TEST(ParseMasterPlaylistResponse) {
        {
            TPlayVideoCommandData commandData;
            const auto error = ParseMasterPlaylistResponse("" /* data */, commandData);
            UNIT_ASSERT_VALUES_EQUAL(error, TPlayError{EPlayError::VIDEOERROR});
        }

        {
            TPlayVideoCommandData commandData;
            const auto error = ParseMasterPlaylistResponse(APPROVED_EMPTY_DATA, commandData);

            // For now, we don't check presense of the "uri" field in
            // the kinopoisk response.
            UNIT_ASSERT_C(!error.Defined(), *error);
        }

        {
            TPlayVideoCommandData commandData;
            const auto error = ParseMasterPlaylistResponse(APPROVED_DATA, commandData);
            UNIT_ASSERT_C(!error.Defined(), *error);
            UNIT_ASSERT_VALUES_EQUAL(commandData->Uri(), "https://strm.yandex.ru/vh-ott-converted/vod-content/"
                                                         "282953532/4c29f978-7a07-c733-b63a-10bc31146625.ism/"
                                                         "manifest.hd.mpd?ottsession="
                                                         "33de11c7d062411da3d9e76295a22e8a");
            UNIT_ASSERT(EqualJson(NSc::TValue::FromJson(commandData->Payload()),
                                  NSc::TValue::FromJson(APPROVED_DATA)["masterPlaylist"]));
        }

        {
            TPlayVideoCommandData commandData;
            const auto error = ParseMasterPlaylistResponse(REJECTED_EMPTY_DATA, commandData);
            UNIT_ASSERT_VALUES_EQUAL(error, TPlayError{EPlayError::VIDEOERROR});
        }

        {
            TPlayVideoCommandData commandData;
            const auto error = ParseMasterPlaylistResponse(REJECTED_DATA, commandData);
            UNIT_ASSERT_VALUES_EQUAL(error, TPlayError{EPlayError::GEO_CONSTRAINT_VIOLATION});
        }
    }

    Y_UNIT_TEST(GetSupportedStreams) {
        const NSc::TValue resultSupportedStreams = NSc::TValue::FromJson(R"(
          {
            "audio": [
              {
                "index": 0,
                "isDefault": false,
                "language": "rus-x-sndk",
                "title": "Русский (Сыендук) "
              },
              {
                "index": 1,
                "isDefault": false,
                "language": "eng",
                "title": "Английский"
              }
            ],
            "drmConfig": {
            },
            "drmType": "widevine",
            "streamType": "DASH",
            "subtitles": [
              {
                "isDefault": false,
                "language": "eng",
                "title": "английские"
              },
              {
                "isDefault": false,
                "language": "rus",
                "title": "русские"
              }
            ]
          }
        )");

        UNIT_ASSERT(EqualJson(
            resultSupportedStreams,
            NAlice::NVideoCommon::GetSupportedStreams(PAYLOAD_ALL_STREAMS).GetRef()));
    }

    Y_UNIT_TEST(GetSortedSkippableFragments) {
        const auto& skippableFragments = TKinopoiskClipsProvider::GetSortedSkippableFragments(
            SKIPPABLE_FRAGMENTS_NOT_SORTED.GetArray());
        UNIT_ASSERT(skippableFragments[0]->EndTime() == 148);
        UNIT_ASSERT(skippableFragments[1]->EndTime() == 149);
        UNIT_ASSERT(skippableFragments[2]->EndTime() == 1322);
    }
}

} // namespace
