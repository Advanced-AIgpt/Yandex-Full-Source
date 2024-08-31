#include <alice/library/json/json.h>
#include <alice/library/video_common/vh_player.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NVideoCommon {

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
    "start_at": 960,
    "onto_category": "film",
    "logo": "http:\/\/avatars.mds.yandex.net\/get-ott\/212840\/2a00000172550ce8255397b4e3d6f9938ddf\/orig",
    "duration": 5920,
    "can_play_on_efir": true,
    "genres": [
        "комедия",
        "семейный"
    ],
    "player_restriction_config": {
        "subtitles_button_enable": false
    },
    "has_schedule": 1,
    "description": "Классика детского кино и один из лучших рождественских фильмов. Сюжет прост: мальчик остается один на Рождество, пока в дом лезут незадачливые грабители. Позже появится сиквел с тем же Маколеем Калкиным и два продолжения уже без него.  Но первая часть – это шедевр. Кстати, в своё время фильм поставил абсолютный рекорд по сборам в жанре комедии, заработав 550 млн долларов."
}
)'");

const TStringBuf VH_PAYLOAD = TStringBuf(R"'(
{
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
    ]
}
)'");

const TStringBuf KINOPOISK_PAYLOAD = TStringBuf(R"'(
{
    "allStreams": [
        {
            "drmConfig": {
                "advanced": {
                    "com.widevine.alpha": {
                        "serverCertificateUrl": "https://widevine-proxy.ott.yandex.ru/certificate"
                    }
                },
                "requestParams": {
                    "contentId": "4ed0391f9e10d314aa0a7de2ea07bf55",
                    "contentTypeId": 20,
                    "expirationTimestamp": 1610664430,
                    "monetizationModel": "SVOD",
                    "productId": 2,
                    "serviceName": "ya-station",
                    "signature": "1246fd3bb2031a633b8192729ead53b67da04ef2",
                    "verificationRequired": true,
                    "version": "V4",
                    "watchSessionId": "1e949151c86b4a22b7faface70d9072f"
                },
                "servers": {
                    "com.widevine.alpha": "https://widevine-proxy.ott.yandex.ru/proxy"
                }
            },
            "drmType": "widevine",
            "streamType": "DASH",
            "uri": "https://strm.yandex.ru/vh-ottenc-converted/vod-content/4ed0391f9e10d314aa0a7de2ea07bf55/7727743x1609533591x02e31241-ae9c-471a-84cc-5ba01665a6a0/dash-cenc/ysign1=f38bfbf6cc77e24e2c97c6dae4e5d9e30d8bd15cfee2ba16cae9634498322d87,from=unknown,pfx,sfx,ts=600da48e/sdr_hd_avc_aac.mpd"
        }
    ],
    "sessionId": "1e949151c86b4a22b7faface70d9072f",
    "streams": {
        "drmConfig": {
            "advanced": {
                "com.widevine.alpha": {
                    "serverCertificateUrl": "https://widevine-proxy.ott.yandex.ru/certificate"
                }
            },
            "requestParams": {
                "contentId": "4ed0391f9e10d314aa0a7de2ea07bf55",
                "contentTypeId": 20,
                "expirationTimestamp": 1610664430,
                "monetizationModel": "SVOD",
                "productId": 2,
                "serviceName": "ya-station",
                "signature": "1246fd3bb2031a633b8192729ead53b67da04ef2",
                "verificationRequired": true,
                "version": "V4",
                "watchSessionId": "1e949151c86b4a22b7faface70d9072f"
            },
            "servers": {
                "com.widevine.alpha":"https://widevine-proxy.ott.yandex.ru/proxy"
            }
        }
    },
    "trackings": {
        "contentTypeId": 20,
        "monetizationModel": "SVOD",
        "puid": null,
        "serviceName": "ya-station",
        "sid": "1e949151c86b4a22b7faface70d9072f",
        "subscriptionType": "YA_PREMIUM",
        "uuid": "4ed0391f9e10d314aa0a7de2ea07bf55",
        "yaUID": null
    },
    "uuid": "4ed0391f9e10d314aa0a7de2ea07bf55",
    "watchProgressPosition": 955
}
)'");

const TStringBuf OTT_PARAMS = TStringBuf(R"'(
{
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
}
)'");

const TStringBuf DRM_CONFIG = TStringBuf(R"'(
{
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
}
)'");

Y_UNIT_TEST_SUITE(VhPlayerTestSuite) {
    Y_UNIT_TEST(TestParseFilm) {
        NJson::TJsonValue content;
        NJson::TJsonValue kinopoiskPayload;
        NJson::TJsonValue vhPayload;
        NJson::TJsonValue ottParams;
        NJson::TJsonValue drmConfig;
        NJson::ReadJsonTree(VH_PLAYER_CONTENT, &content);
        NJson::ReadJsonTree(KINOPOISK_PAYLOAD, &kinopoiskPayload);
        NJson::ReadJsonTree(VH_PAYLOAD, &vhPayload);
        NJson::ReadJsonTree(OTT_PARAMS, &ottParams);
        NJson::ReadJsonTree(DRM_CONFIG, &drmConfig);

        TVhPlayerData vhPlayerData;
        // Parsing
        UNIT_ASSERT_VALUES_EQUAL(vhPlayerData.ParseJsonDoc(content), true);
        UNIT_ASSERT_VALUES_EQUAL(vhPlayerData.IsFilled(), true);
        // Content type
        UNIT_ASSERT_VALUES_EQUAL(vhPlayerData.IsFilm(), true);
        UNIT_ASSERT_VALUES_EQUAL(vhPlayerData.IsTvShow(), false);
        UNIT_ASSERT_VALUES_EQUAL(vhPlayerData.IsTvShowEpisode(), false);
        UNIT_ASSERT_VALUES_EQUAL(vhPlayerData.IsTvStream(), false);
        // Content provider
        UNIT_ASSERT_VALUES_EQUAL(vhPlayerData.IsKinopoisk(), true);
        UNIT_ASSERT_VALUES_EQUAL(vhPlayerData.IsVh(), false);
        // Playable data
        UNIT_ASSERT_VALUES_EQUAL(vhPlayerData.IsPlayableVhPlayerData(), true);
        // Time
        ui64 startTime = vhPlayerData.GetStartAt(/*timeNow*/ 1610646213);
        UNIT_ASSERT_VALUES_EQUAL(startTime, 955);
        // Payload
        UNIT_ASSERT_VALUES_EQUAL(vhPlayerData.MakeVhPayload(), vhPlayerData.Payload);
        UNIT_ASSERT_VALUES_EQUAL(vhPlayerData.MakePayload(startTime), NAlice::JsonToString(kinopoiskPayload));
        UNIT_ASSERT_VALUES_EQUAL(vhPlayerData.MakePayload(startTime), vhPlayerData.MakeKinopoiskPayload(startTime));
        // Fields
        UNIT_ASSERT_VALUES_EQUAL(vhPlayerData.Title, "Один дома");
        UNIT_ASSERT_VALUES_EQUAL(vhPlayerData.Uuid, "4ed0391f9e10d314aa0a7de2ea07bf55");
        UNIT_ASSERT_VALUES_EQUAL(vhPlayerData.PlayUri, "https://strm.yandex.ru/vh-ottenc-converted/vod-content/4ed0391f9e10d314aa0a7de2ea07bf55/7727743x1609533591x02e31241-ae9c-471a-84cc-5ba01665a6a0/dash-cenc/ysign1=f38bfbf6cc77e24e2c97c6dae4e5d9e30d8bd15cfee2ba16cae9634498322d87,from=unknown,pfx,sfx,ts=600da48e/sdr_hd_avc_aac.mpd");
        UNIT_ASSERT_VALUES_EQUAL(vhPlayerData.StreamType, "DASH");
        UNIT_ASSERT_VALUES_EQUAL(vhPlayerData.TvShowSeasonId, "46a299cbc63c865db2e7cc49296e4a5e");
        UNIT_ASSERT_VALUES_EQUAL(vhPlayerData.TvShowItemId, "");
        UNIT_ASSERT_VALUES_EQUAL(vhPlayerData.Payload, vhPayload.GetStringRobust());
        UNIT_ASSERT_VALUES_EQUAL(vhPlayerData.Description, "Классика детского кино и один из лучших рождественских фильмов. Сюжет прост: мальчик остается один на Рождество, пока в дом лезут незадачливые грабители. Позже появится сиквел с тем же Маколеем Калкиным и два продолжения уже без него.  Но первая часть – это шедевр. Кстати, в своё время фильм поставил абсолютный рекорд по сборам в жанре комедии, заработав 550 млн долларов.");
        UNIT_ASSERT_VALUES_EQUAL(vhPlayerData.Duration, 5920);
        UNIT_ASSERT_VALUES_EQUAL(vhPlayerData.StartAt, 960);
        UNIT_ASSERT_VALUES_EQUAL(vhPlayerData.StartTimestamp, 0);
        UNIT_ASSERT_VALUES_EQUAL(vhPlayerData.EndTimestamp, 0);
        UNIT_ASSERT_VALUES_EQUAL(vhPlayerData.VideoType, EContentType::Movie);
        UNIT_ASSERT_VALUES_EQUAL(vhPlayerData.SessionId, "1e949151c86b4a22b7faface70d9072f");
        UNIT_ASSERT_VALUES_EQUAL(vhPlayerData.RestrictionAge, 0);
        UNIT_ASSERT_VALUES_EQUAL(vhPlayerData.ReleaseYear, 1990);
        UNIT_ASSERT_VALUES_EQUAL(vhPlayerData.Genre, "комедия, семейный");
        UNIT_ASSERT_VALUES_EQUAL(vhPlayerData.Directors, "Крис Коламбус");
        UNIT_ASSERT_VALUES_EQUAL(vhPlayerData.Actors, "Маколей Калкин, Джо Пеши, Дэниел Стерн");
        UNIT_ASSERT_VALUES_EQUAL(vhPlayerData.RatingKP, 8.239999771);
        UNIT_ASSERT_VALUES_EQUAL(vhPlayerData.IsPaidContent, true);
        UNIT_ASSERT_VALUES_EQUAL(vhPlayerData.HasActiveLicense, true);
        UNIT_ASSERT_VALUES_EQUAL(vhPlayerData.OntoId, "ruw110723");
        UNIT_ASSERT_VALUES_EQUAL(vhPlayerData.DrmConfig, drmConfig);
        UNIT_ASSERT_VALUES_EQUAL(vhPlayerData.OttParams, ottParams);
        UNIT_ASSERT_VALUES_EQUAL(vhPlayerData.PlayerRestrictionConfig.subtitlesbuttonenable(), false);
        // Audio and subtitles
        using ASOS = NAlice::TAudioStreamOrSubtitle;
        ui32 counter = 1;
        auto addAsos = [&counter](TVector<ASOS>& asoses, const TString& language, const TString& title) {
            ASOS asos;
            asos.SetLanguage(language);
            asos.SetTitle(title);
            asos.SetIndex(counter++);
            asoses.push_back(asos);
        };

        TVector<ASOS> audio;
        addAsos(audio, "rus", "Русский");
        addAsos(audio, "eng", "Английский");
        TVector<ASOS> subtitles;
        addAsos(subtitles, ToString(LANGUAGE_SUBTITLE_OFF), "Выключены");
        addAsos(subtitles, "rus", "Русские");

        auto checkAudioOrSubtitles = [](const TVector<ASOS>& asoses1, const TVector<ASOS>& asoses2) {
            UNIT_ASSERT_VALUES_EQUAL(asoses1.size(), asoses2.size());
            for(size_t i = 0; i < asoses1.size(); ++i) {
                const auto& asos1 = asoses1[i];
                const auto& asos2 = asoses2[i];
                UNIT_ASSERT_VALUES_EQUAL(asos1.language(), asos2.language());
                UNIT_ASSERT_VALUES_EQUAL(asos1.title(), asos2.title());
                UNIT_ASSERT_VALUES_EQUAL(asos1.index(), asos2.index());
            }
        };
        checkAudioOrSubtitles(vhPlayerData.AudioStreams, audio);
        checkAudioOrSubtitles(vhPlayerData.Subtitles, subtitles);
    }

    void AssertPlayerRestictionConfig(const NJson::TJsonValue& content, bool subtitlesButtonEnable) {
        TVhPlayerData vhPlayerData;
        UNIT_ASSERT_VALUES_EQUAL(vhPlayerData.ParseJsonDoc(content), true);
        UNIT_ASSERT_VALUES_EQUAL(vhPlayerData.IsFilled(), true);
        UNIT_ASSERT_VALUES_EQUAL(vhPlayerData.PlayerRestrictionConfig.subtitlesbuttonenable(), subtitlesButtonEnable);
    }

    Y_UNIT_TEST(TestParsePlayerRestictionConfig) {
        NJson::TJsonValue content;
        NJson::ReadJsonTree(VH_PLAYER_CONTENT, &content);
        // with stricted value
        AssertPlayerRestictionConfig(content, false);
        content["player_restriction_config"]["subtitles_button_enable"] = true;
        AssertPlayerRestictionConfig(content, true);
        // default
        content["player_restriction_config"].EraseValue("subtitles_button_enable");
        AssertPlayerRestictionConfig(content, true);
        content.EraseValue("player_restriction_config");
        AssertPlayerRestictionConfig(content, true);
        // with ott style value
        content["playerRestrictionConfig"]["subtitlesButtonEnable"] = false;
        AssertPlayerRestictionConfig(content, false);
        content["playerRestrictionConfig"]["subtitlesButtonEnable"] = true;
        AssertPlayerRestictionConfig(content, true);
    }
}

} // namespace

}
