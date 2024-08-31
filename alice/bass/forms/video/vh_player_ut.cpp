#include "vh_player.h"

#include <alice/bass/ut/helpers.h>
#include <alice/library/json/json.h>
#include <alice/library/video_common/defs.h>

#include <library/cpp/scheme/scheme.h>

namespace {

const TStringBuf VH_PLAYER_CONTENT = TStringBuf(R"'(
{
    "parent_id": "46a299cbc63c865db2e7cc49296e4a5e",
    "catchup_age": 0,
    "licenses": [
        {
            "total_purchases": 7,
            "monetizationModel": "TVOD",
            "price": 149,
            "price_with_discount": 104
        },
        {
            "monetizationModel": "SVOD",
            "subscriptionTypes": [
                "YA_PLUS_SUPER",
                "YA_PLUS_3M",
                "YA_PLUS",
                "YA_PREMIUM",
                "YA_PLUS_KP",
                "KP_BASIC"
            ]
        },
        {
            "total_purchases": 7,
            "monetizationModel": "EST",
            "price":299,"price_with_discount": 209
        }
    ],
    "supertag": "subscription",
    "content_url": "https:\/\/strm.yandex.ru\/vh-ottenc-converted\/vod-content\/4ed0391f9e10d314aa0a7de2ea07bf55\/7727743x1609254759x27121ffa-28e9-4a78-8f28-ddc53ec9ac31\/mss-pr.ism\/manifest_sdr_hd_avc_aac.ismc",
    "title": "Один дома",
    "supertag_title": "Кинопоиск HD",
    "stream": {
        "trackings": {
            "trackingEvents": []
        },
        "withCredentials": true
    },
    "height": 1080,
    "rating_kp": 8.239999771,
    "actors": "Маколей Калкин, Джо Пеши, Дэниел Стерн",
    "thumbnail": "\/\/avatars.mds.yandex.net\/get-vh\/3482920\/7474298760445773361-QTH2Vdx3B9xj2hnw30h89w-1595514349\/orig",
    "content_id": "4ed0391f9e10d314aa0a7de2ea07bf55",
    "skippableFragments": [
        {
            "endTime": null,
            "type": null,
            "result": "DELETE",
            "startTime": null
        },
        {
            "endTime": 5920,
            "type": "credits",
            "result": "OK",
            "startTime": 5698
        }
    ],
    "stream_quality": "hd",
    "cover": "\/\/avatars.mds.yandex.net\/get-ott\/236744\/2a0000017672cc1b8d4660336ae565af15fd\/orig",
    "watermark_logo": "https:\/\/avatars.mds.yandex.net\/get-ott\/239697\/2a00000168e2708c53767151fbe7f6e403fc\/orig",
    "release_year": 1990,
    "onto_id": "ruw110723",
    "views_count": 574701,
    "can_play_on_station": true,
    "directors": "Крис Коламбус",
    "ya_plus": [
        "YA_PLUS_SUPER",
        "YA_PLUS_3M",
        "YA_PLUS",
        "YA_PREMIUM",
        "YA_PLUS_KP",
        "KP_BASIC"
    ],
    "onto_otype": "Film\/Film",
    "producers": "Тарквин Готч, Джон Хьюз, Марк Левинсон",
    "update_time": 1609676676,
    "has_cachup": 1,
    "ottParams": {
        "monetizationModel": "SVOD",
        "serviceName": "ya-station",
        "contentTypeID": 20,
        "kpId": "8124",
        "licenses": [
            {
                "monetizationModel": "SVOD",
                "active": true,
                "primary": true,
                "purchaseTag": "plus"
            }
        ],
        "reqid": "1610642830130358-16537173036132084145",
        "yandexuid": "3478013221610642830",
        "uuid": "4ed0391f9e10d314aa0a7de2ea07bf55",
        "from": "unknown",
        "subscriptionType": "YA_PREMIUM"
    },
    "countries": "США",
    "dislikes": 96,
    "deep_hd": false,
    "is_special_project": false,
    "short_description": "Мальчик-озорник задает жару грабителям. Лучшая комедия для создания праздничного настроения у всей семьи",
    "yastation_cast_cfg": {
        "provider_item_id": "https:\/\/frontend.vh.yandex.ru\/player\/4ed0391f9e10d314aa0a7de2ea07bf55",
        "player_id": "vh"
    },
    "computed_title": "Один дома (1990) — комедия, HD",
    "has_vod": 0,
    "width": 1920,
    "dvr": 0,
    "likes": 369,
    "ya_video_preview": "https:\/\/video-preview.s3.yandex.net\/vh\/7474298760445773361_vmaf-preview-720.mp4",
    "streams": [
        {
            "stream_type": "DASH",
            "url": "https:\/\/strm.yandex.ru\/vh-ottenc-converted\/vod-content\/4ed0391f9e10d314aa0a7de2ea07bf55\/7727743x1609533591x02e31241-ae9c-471a-84cc-5ba01665a6a0\/dash-cenc\/ysign1=f38bfbf6cc77e24e2c97c6dae4e5d9e30d8bd15cfee2ba16cae9634498322d87,from=unknown,pfx,sfx,ts=600da48e\/sdr_hd_avc_aac.mpd",
            "audio": [
                {
                    "language": "rus",
                    "title": "Русский"
                },
                {
                    "language": "eng",
                    "title": "Английский"
                }
            ],
            "drmConfig": {
                "servers": {
                    "com.widevine.alpha": "https:\/\/widevine-proxy.ott.yandex.ru\/proxy"
                },
                "advanced": {
                    "com.widevine.alpha": {
                    "serverCertificateUrl": "https:\/\/widevine-proxy.ott.yandex.ru\/certificate"
                    }
                },
                "requestParams": {
                    "verificationRequired": true,
                    "monetizationModel": "SVOD",
                    "contentId": "4ed0391f9e10d314aa0a7de2ea07bf55",
                    "productId": 2,
                    "serviceName": "ya-station",
                    "expirationTimestamp": 1610664430,
                    "contentTypeId": 20,
                    "watchSessionId": "1e949151c86b4a22b7faface70d9072f",
                    "signature": "1246fd3bb2031a633b8192729ead53b67da04ef2",
                    "version": "V4"
                }
            },
            "subtitles": [
                {
                    "language": "rus",
                    "title": "Русские"
                }
            ]
        }
    ],
    "onto_poster": "\/\/avatars.mds.yandex.net\/get-ott\/200035\/2a000001612cc5ed5494f93ae6841af30594\/orig",
    "content_type_name": "vod-episode",
    "restriction_age": 0,
    "blacked": 0,
    "progress": 950,
    "onto_category": "film",
    "logo": "http:\/\/avatars.mds.yandex.net\/get-ott\/212840\/2a00000172550ce8255397b4e3d6f9938ddf\/orig",
    "duration": 5920,
    "can_play_on_efir": true,
    "genres": [
        "комедия",
        "семейный"
    ],
    "player_restriction_config": {
        "subtitles_button_enable": true
    },
    "has_schedule": 1,
    "description": "Классика детского кино и один из лучших рождественских фильмов. Сюжет прост: мальчик остается один на Рождество, пока в дом лезут незадачливые грабители. Позже появится сиквел с тем же Маколеем Калкиным и два продолжения уже без него.  Но первая часть – это шедевр. Кстати, в своё время фильм поставил абсолютный рекорд по сборам в жанре комедии, заработав 550 млн долларов."
}
)'");

NBASS::NVideo::TVideoItem MakeSchemeVideoItemBrute(NAlice::TVideoItem item) {
    NBASS::NVideo::TVideoItem bassItem(NSc::TValue::FromJsonValue(NAlice::JsonFromProto(item)));
    if (item.GetMinAge() == 0) {
        bassItem->MinAge() = 0;
    }
    NBASS::NVideo::FillAgeLimit(bassItem);
    return bassItem;
}

Y_UNIT_TEST_SUITE(VhPlayerBassTestSuite) {
    Y_UNIT_TEST(TestScemeItem) {
        NJson::TJsonValue content;
        NJson::ReadJsonTree(VH_PLAYER_CONTENT, &content);

        TVhPlayerData vhPlayerData;
        vhPlayerData.ParseJsonDoc(content);
        auto item = NBASS::NVideo::MakeSchemeVideoItem(vhPlayerData);
        auto bruteItem = MakeSchemeVideoItemBrute(vhPlayerData.MakeProtoVideoItem());
        UNIT_ASSERT_VALUES_EQUAL(item->GetRawValue()->ToJson(), bruteItem->GetRawValue()->ToJson());
#define COMPARE_FIELD(field) UNIT_ASSERT_VALUES_EQUAL(item->field(), bruteItem->field());
#define COMPARE_ARRAY_FIELD(array, index, field) UNIT_ASSERT_VALUES_EQUAL(item->array()[index].field(), bruteItem->array()[index].field());
#define COMPARE_PROVIDER_INFO_FIELD(field) COMPARE_ARRAY_FIELD(ProviderInfo, 0, field)
        COMPARE_FIELD(Name)
        COMPARE_FIELD(ProviderItemId)
        COMPARE_PROVIDER_INFO_FIELD(ProviderItemId)
        COMPARE_FIELD(Type)
        COMPARE_PROVIDER_INFO_FIELD(Type)
        COMPARE_FIELD(Description)
        COMPARE_FIELD(Duration)
        COMPARE_FIELD(ProviderName)
        COMPARE_PROVIDER_INFO_FIELD(ProviderName)
        COMPARE_FIELD(PlayUri)
        COMPARE_FIELD(MinAge)
        COMPARE_FIELD(AgeLimit)
        COMPARE_FIELD(Available)
        COMPARE_PROVIDER_INFO_FIELD(Available)
        COMPARE_FIELD(Entref)
        COMPARE_FIELD(Genre)
        COMPARE_FIELD(Directors)
        COMPARE_FIELD(Actors)
        COMPARE_FIELD(Rating)
        COMPARE_FIELD(ReleaseYear)
        COMPARE_FIELD(CoverUrl16X9)
        COMPARE_FIELD(CoverUrl2X3)
        COMPARE_FIELD(ThumbnailUrl16X9)
        COMPARE_FIELD(Episode)
        COMPARE_PROVIDER_INFO_FIELD(Episode)
        COMPARE_FIELD(Season)
        COMPARE_PROVIDER_INFO_FIELD(Season)
        COMPARE_FIELD(TvShowSeasonId)
        COMPARE_PROVIDER_INFO_FIELD(TvShowSeasonId)
        COMPARE_FIELD(TvShowItemId)
        COMPARE_PROVIDER_INFO_FIELD(TvShowItemId)
        COMPARE_FIELD(PlayerRestrictionConfig().SubtitlesButtonEnable)

#define COMPARE_AUDIO_STREAM_OR_SUBTITLES(array)            \
        for (size_t i = 0; i < item->array().Size(); ++i) { \
            COMPARE_ARRAY_FIELD(array, i, Language)         \
            COMPARE_ARRAY_FIELD(array, i, Title)            \
            COMPARE_ARRAY_FIELD(array, i, Index)            \
            COMPARE_ARRAY_FIELD(array, i, Suggest)          \
        }

        COMPARE_FIELD(AudioStreams().Size)
        COMPARE_AUDIO_STREAM_OR_SUBTITLES(AudioStreams)
        COMPARE_FIELD(Subtitles().Size)
        COMPARE_AUDIO_STREAM_OR_SUBTITLES(Subtitles)
    }
}

} // namespace
