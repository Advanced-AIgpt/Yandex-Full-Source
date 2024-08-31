#include "play_video.h"
#include "billing.h"
#include "video_provider.h"

#include <alice/bass/ut/helpers.h>
#include <alice/library/video_common/defs.h>

#include <library/cpp/scheme/scheme.h>

using namespace NTestingHelpers;

namespace {

const NSc::TValue CONTEXT_PLAY_VIDEO = NSc::TValue::FromJson(R"'(
{
  "form": {
    "name": "personal_assistant.scenarios.video_play"
  },
  "meta": {
    "client_id": "ru.yandex.quasar.app/1.0/ ( ;  )",
    "client_info": {
      "app_id": "ru.yandex.quasar.app/1.0"
    },
    "device_state": {
      "last_watched": {
        "tv_shows": [{
          "tv_show_item": {
            "provider_name": "kinopoisk",
            "provider_item_id": "4c96bc227a377c8aa98da3789416c811"
          },
          "item": {
            "audio_language": "en",
            "subtitles_language": "ru"
          }
        }]
      }
    },
    "epoch": 1590389680,
    "tz": "Europe/Moscow"
  }
}
)'");

const NSc::TValue CONTEXT_PLAY_NEXT_VIDEO = NSc::TValue::FromJson(R"'(
{
  "form": {
    "name": "personal_assistant.scenarios.video_play"
  },
  "meta": {
    "client_id": "ru.yandex.quasar.app/1.0/ ( ;  )",
    "client_info": {
      "app_id": "ru.yandex.quasar.app/1.0"
    },
    "device_state": {
      "video": {
        "current_screen": "video_player",
        "currently_playing": {
          "audio_language" : "rus-x-sndk18",
          "subtitles_language" : "eng",
          "item": {
            "subtitles_button_enable" : true,
            "tv_show_item_id" : "4c96bc227a377c8aa98da3789416c811"
          }
        }
      }
    },
    "epoch": 1590389680,
    "tz": "Europe/Moscow"
  }
}
)'");

const NSc::TValue PAYLOAD = NSc::TValue::FromJson(R"(
{
    "allStreams": [
        {
            "audio": [
                {
                    "index": 0,
                    "isDefault": false,
                    "language": "rus",
                    "title": "Русский"
                },
                {
                    "index": 1,
                    "isDefault": false,
                    "language": "eng",
                    "title": "Английский"
                }
            ],
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
                "language": "rus",
                "title": "Русский"
            },
            {
                "index": 1,
                "isDefault": false,
                "language": "eng",
                "title": "Английский"
            }
        ],
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

NBASS::NVideo::TVideoItem CreateVideoItem(
    TStringBuf url,
    TStringBuf providerItemId,
    TStringBuf tvShowItemId,
    TStringBuf title,
    TStringBuf description,
    const TMaybe<ui32>& duration = Nothing(),
    const TMaybe<ui32>& seasonNumber = Nothing(),
    const TMaybe<ui32>& episodeNumber = Nothing())
{
    NBASS::NVideo::TVideoItem item{};
    item->PlayUri() = url;

    item->ProviderName() = "kinopoisk";
    item->ProviderItemId() = providerItemId;
    item->TvShowItemId() = tvShowItemId;

    item->Type() = "tv_show";
    item->Available() = true;

    item->Name() = title;
    item->Description() = description;

    if (duration.Defined()) {
        item->Duration() = duration.GetRef();
    }

    if (seasonNumber.Defined()) {
        item->Season() = seasonNumber.GetRef();
    }

    if (episodeNumber.Defined()) {
        item->Episode() = episodeNumber.GetRef();
    }
    return item;
}

Y_UNIT_TEST_SUITE(VideoPlayVideo) {
    // New version
    Y_UNIT_TEST(PlayVideo) {
        auto ctx = MakeContext(CONTEXT_PLAY_VIDEO);

        auto curr = CreateVideoItem(
            "" /* url */,
            "453f5cd7efb1bb06a468d9a120448afe" /* providerItemId */,
            "4c96bc227a377c8aa98da3789416c811" /* tvShowItemId */,
            "8. Дефицит самоклеющихся утят" /* title */,
            "123" /* description */,
            1230 /* duration */,
            3 /* seasonNumber */,
            8 /* episodeNumber */);

        auto next = CreateVideoItem(
            "" /* url */,
            "4d721ba2f956660e9af8c54a42c28d7f" /* providerItemId */,
            "4c96bc227a377c8aa98da3789416c811" /* tvShowItemId */,
            "9. Формула мести" /* title */,
            "123" /* description */,
            1130 /* duration */,
            3 /* seasonNumber */,
            9 /* episodeNumber */);

        auto parent = CreateVideoItem(
            "" /* url */,
            "4c96bc227a377c8aa98da3789416c811" /* providerItemId */,
            "4c96bc227a377c8aa98da3789416c811" /* tvShowItemId */,
            "Теория большого взрыва" /* title */,
            "123" /* description */);

        NBASS::NVideo::TPlayData billingData{
            "kinopoisk" /* ProviderName */,
            PAYLOAD,
            "" /* Url */,
            Nothing() /* SessionToken */};

        const auto provider = NBASS::NVideo::CreateProvider(TStringBuf("kinopoisk"), *ctx);

        UNIT_ASSERT(!NBASS::NVideo::PlayVideo(
            curr.Scheme(),
            next.Scheme(),
            parent.Scheme(),
            *provider,
            *ctx,
            billingData,
            Nothing()).Defined());

        UNIT_ASSERT(ctx->GetCommand(NAlice::NVideoCommon::COMMAND_VIDEO_PLAY)->Get("data").Get("audio_language") == "en");
        UNIT_ASSERT(ctx->GetCommand(NAlice::NVideoCommon::COMMAND_VIDEO_PLAY)->Get("data").Get("subtitles_language") == "ru");
    }

    Y_UNIT_TEST(PlayNextVideo) {
        auto ctx = MakeContext(CONTEXT_PLAY_NEXT_VIDEO);

        auto curr = CreateVideoItem(
            "" /* url */,
            "453f5cd7efb1bb06a468d9a120448afe" /* providerItemId */,
            "4c96bc227a377c8aa98da3789416c811" /* tvShowItemId */,
            "8. Дефицит самоклеющихся утят" /* title */,
            "123" /* description */,
            1230 /* duration */,
            3 /* seasonNumber */,
            8 /* episodeNumber */);

        auto next = CreateVideoItem(
            "" /* url */,
            "4d721ba2f956660e9af8c54a42c28d7f" /* providerItemId */,
            "4c96bc227a377c8aa98da3789416c811" /* tvShowItemId */,
            "9. Формула мести" /* title */,
            "123" /* description */,
            1130 /* duration */,
            3 /* seasonNumber */,
            9 /* episodeNumber */);

        auto parent = CreateVideoItem(
            "" /* url */,
            "4c96bc227a377c8aa98da3789416c811" /* providerItemId */,
            "4c96bc227a377c8aa98da3789416c811" /* tvShowItemId */,
            "Теория большого взрыва" /* title */,
            "123" /* description */);

        NBASS::NVideo::TPlayData billingData{
            "kinopoisk" /* ProviderName */,
            PAYLOAD,
            "" /* Url */,
            Nothing() /* SessionToken */};

        const auto provider = NBASS::NVideo::CreateProvider(TStringBuf("kinopoisk"), *ctx);

        UNIT_ASSERT(!NBASS::NVideo::PlayVideo(
            curr.Scheme(),
            next.Scheme(),
            parent.Scheme(),
            *provider,
            *ctx,
            billingData,
            Nothing()).Defined());

        UNIT_ASSERT(ctx->GetCommand(NAlice::NVideoCommon::COMMAND_VIDEO_PLAY)->Get("data").Get("audio_language") == "rus-x-sndk18");
        UNIT_ASSERT(ctx->GetCommand(NAlice::NVideoCommon::COMMAND_VIDEO_PLAY)->Get("data").Get("subtitles_language") == "eng");
    }
}

} // namespace
