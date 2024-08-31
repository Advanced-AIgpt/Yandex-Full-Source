#include <alice/library/search_result_parser/video/parsers.h>
#include <alice/library/search_result_parser/video/matcher_util.h>
#include <alice/library/search_result_parser/video/parser_util.h>
#include <alice/library/json/json.h>
#include <alice/library/proto/proto_adapter.h>
#include <alice/library/unittest/mock_sensors.h>
#include <alice/hollywood/library/scenarios/video/web_search_helpers.h>
#include <alice/hollywood/library/scenarios/video/search_metrics.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/stream/file.h>

// declaration of 'private' SearchMetrics functions
namespace NAlice::NHollywoodFw::NVideo::NSearchMetrics::Monitoring::Web {
    void TrackNoDatasource(NMetrics::ISensors& sensors, TRTLogger& logger);
    void TrackNoSnippet(NMetrics::ISensors& sensors, TRTLogger& logger);
    void TrackWebSnippet(NMetrics::ISensors& sensors, TRTLogger& logger, const GStruct& snippet);
}
 
namespace NAlice::NHollywood::NVideo {
    namespace {
        NJson::TJsonValue getFixture(const TString path) {
            TFileInput inputData(path);
            return std::move(JsonFromString(inputData.ReadAll()));
        }
    }

    Y_UNIT_TEST_SUITE(VideoScenario) {
            Y_UNIT_TEST(MakeCinemaData) {
                const NJson::TJsonValue& document = getFixture("fixtures/cinemaData.json");
                auto cinemaData = MakeCinemaData(document);

                Y_ASSERT(cinemaData.size() == 1);
                {
                    auto cinema = cinemaData[0];
                    Y_ASSERT(cinema.VariantsSize() == 1);
                    Y_ASSERT(cinema.GetVariants(0).GetPrice() == 799);
                    Y_ASSERT(cinema.GetVariants(0).GetType() == "svod");
                    Y_ASSERT(cinema.GetFavicon() == "https://avatars.mds.yandex.net/get-ott/239697/7713e586-17d1-42d1-ac62-53e9ef1e70c3/");
                    Y_ASSERT(cinema.GetCinemaName() == "Okko");
                    Y_ASSERT(cinema.GetCode() == "okko");
                    Y_ASSERT(cinema.GetLink() == "https://okko.tv/serial/game-of-thrones?utm_medium=referral&utm_source=yandex_search&utm_campaign=new_search_feed");
                    Y_ASSERT(cinema.GetTvPackageName() == "tv.okko.androidtv");
                    Y_ASSERT(cinema.GetTvFallbackLink() == "home-app://market_item?package=tv.okko.androidtv");
                    Y_ASSERT(cinema.GetTvDeepLink() == "yotaplay://movie?uid=1e36b10b-ed8f-cf3b-cbe1-9d88123ac2aa&type=serial&action=play");
                }
            }

            Y_UNIT_TEST(MakeCinemaDataMore) {
                const NJson::TJsonValue& document = getFixture("fixtures/CinemaDataFaked.json");
                auto cinemaData = MakeCinemaData(document);

                Y_ASSERT(cinemaData.size() == 2);
                {
                    auto cinema = cinemaData[0];
                    Y_ASSERT(cinema.VariantsSize() == 3);
                    Y_ASSERT(cinema.GetVariants(0).GetPrice() == 349);
                    Y_ASSERT(cinema.GetVariants(0).GetType() == "est");
                    Y_ASSERT(cinema.GetVariants(1).GetPrice() == 379);
                    Y_ASSERT(cinema.GetVariants(1).GetType() == "svod");
                    Y_ASSERT(cinema.GetVariants(2).GetPrice() == 150);
                    Y_ASSERT(cinema.GetVariants(2).GetType() == "fvod");
                    Y_ASSERT(cinema.GetCinemaName() == "Wink");
                    Y_ASSERT(cinema.GetCode() == "wink");
                    Y_ASSERT(cinema.GetLink() == "https://wink.ru/media_items/96182315?utm_source=yandex&utm_medium=koldunschick&utm_content=name");
                    Y_ASSERT(cinema.GetTvPackageName() == "ru.rt.video.app.tv");
                }
                {
                    auto cinema = cinemaData[1];
                    Y_ASSERT(cinema.VariantsSize() == 1);
                    Y_ASSERT(cinema.GetVariants(0).GetPrice() == 799);
                    Y_ASSERT(cinema.GetVariants(0).GetType() == "svod");
                    Y_ASSERT(cinema.GetFavicon() == "https://avatars.mds.yandex.net/get-ott/239697/7713e586-17d1-42d1-ac62-53e9ef1e70c3/");
                    Y_ASSERT(cinema.GetCinemaName() == "Okko");
                    Y_ASSERT(cinema.GetCode() == "okko");
                    Y_ASSERT(cinema.GetLink() == "https://okko.tv/serial/game-of-thrones?utm_medium=referral&utm_source=yandex_search&utm_campaign=new_search_feed");
                    Y_ASSERT(cinema.GetTvPackageName() == "tv.okko.androidtv");
                    Y_ASSERT(cinema.GetTvFallbackLink() == "home-app://market_item?package=tv.okko.androidtv");
                }
            }

            Y_UNIT_TEST(TestCinemaDataFromRichInfo) {
                const TString smychokCinemaData = R"(
                {
                    "keyart": "//avatars.mds.yandex.net/get-ott/223007/2a0000016fe6368a9918a4a0f2f91bde9be0/orig",
                    "cinemas": [
                        {
                            "embed_url": "",
                            "cinema_name": "START",
                            "tv_package_name": "ru.start.androidmobile",
                            "variants": [
                                {
                                    "subscription_name": "",
                                    "price": 399,
                                    "type": "svod",
                                    "quality": "",
                                    "embed_url": ""
                                }
                            ],
                            "code": "start",
                            "tv_fallback_link": "home-app://market_item?package=ru.start.androidmobile",
                            "favicon": "//avatars.mds.yandex.net/get-ott/239697/1a632675-0d99-4268-bd5e-d5f3dd800174/orig",
                            "link": "https://start.ru/watch/smychok?utm_source=kinopoisk&utm_medium=feed_watch&utm_campaign=smychok",
                            "tv_deeplink": "startru://start/series/smychok",
                            "duration": 0,
                            "hide_price": false
                        },
                        {
                            "cinema_name": "Okko",
                            "link": "https://okko.tv/serial/smychok?utm_medium=referral&utm_source=yandex_search&utm_campaign=new_search_feed",
                            "tv_package_name": "tv.okko.androidtv",
                            "embed_url": "",
                            "code": "okko",
                            "tv_fallback_link": "home-app://market_item?package=tv.okko.androidtv",
                            "tv_deeplink": "yotaplay://movie?uid=2abb6b0f-2b90-409e-a70f-118df477b3f3&type=serial&action=play",
                            "duration": 0,
                            "variants": [
                                {
                                    "embed_url": "",
                                    "price": 799,
                                    "type": "svod",
                                    "quality": "",
                                    "subscription_name": ""
                                }
                            ],
                            "favicon": "//avatars.mds.yandex.net/get-ott/239697/7713e586-17d1-42d1-ac62-53e9ef1e70c3/orig",
                            "hide_price": false
                        }
                    ],
                    "list": null,
                    "methods_available": [
                        "svod"
                    ],
                    "thumbnail": "//avatars.mds.yandex.net/get-ott/239697/ae2e101eb8227a2e5aef89654e01dd2b44463fb1/orig",
                    "poster": {
                        "horizontal": "",
                        "vertical": "//avatars.mds.yandex.net/get-ott/1534341/ce0750d5d8ca4db20dcb3ffec2515b0c66cac03c/orig"
                    },
                    "age_restrictions": 18
                }
                )";
                const NJson::TJsonValue& document = JsonFromString(smychokCinemaData);
                auto cinemaData = MakeCinemaData(document);
                Y_ASSERT(cinemaData.size() == 2);
                {
                    auto cinema = cinemaData[0];
                    Y_ASSERT(cinema.GetTvDeepLink() == "startru://start/series/smychok");
                    Y_ASSERT(cinema.GetCinemaName() == "START");
                    Y_ASSERT(cinema.GetTvPackageName() == "ru.start.androidmobile");
                    Y_ASSERT(cinema.GetKeyArt().GetBaseUrl() == "https://avatars.mds.yandex.net/get-ott/223007/2a0000016fe6368a9918a4a0f2f91bde9be0/");
                    Y_ASSERT(cinema.GetHidePrice() == false);
                }
                {
                    auto cinema = cinemaData[1];
                    Y_ASSERT(cinema.GetTvDeepLink() == "yotaplay://movie?uid=2abb6b0f-2b90-409e-a70f-118df477b3f3&type=serial&action=play");
                    Y_ASSERT(cinema.GetCinemaName() == "Okko");
                    Y_ASSERT(cinema.GetTvPackageName() == "tv.okko.androidtv");
                    Y_ASSERT(cinema.GetKeyArt().GetBaseUrl() == "https://avatars.mds.yandex.net/get-ott/223007/2a0000016fe6368a9918a4a0f2f91bde9be0/");
                    Y_ASSERT(cinema.GetHidePrice() == false);
                }
            }

            Y_UNIT_TEST(ParseClips) {
                const NJson::TJsonValue& document = getFixture("fixtures/multiki.json");

                NProtoBuf::RepeatedPtrField<NTv::TCarouselItemWrapper>  webItems = SearchResultParser::ParseClips(document, TRTLogger::NullLogger());

                Y_ASSERT(webItems.size() == 20);

                const TWebVideoItem& firstItem = webItems[0].GetSearchVideoItem();
                Y_ASSERT(firstItem.GetId() == "BtweaJFeqLc");
                Y_ASSERT(firstItem.GetContentType() == "video");
                Y_ASSERT(firstItem.GetTitle() == "Маша и Медведь - 💥 Новая Серия! 🐻 Живая шляпа 🎃 Коллекция мультиков для детей про Машу");
                Y_ASSERT(firstItem.GetPlayerId() == "youtube");
                Y_ASSERT(firstItem.GetThumbnail().GetBaseUrl() == "https://avatars.mds.yandex.net/get-vthumb/774126/8136da055769db874d1c5f1ef57363c8/");
                Y_ASSERT(firstItem.GetThumbnail().GetSizes().size() == 5);
                Y_ASSERT(firstItem.GetThumbnail().GetSizes()[0] == "160x90");
                Y_ASSERT(firstItem.GetThumbnail().GetSizes()[1] == "720x360");
                Y_ASSERT(firstItem.GetThumbnail().GetSizes()[2] == "960x540");
                Y_ASSERT(firstItem.GetThumbnail().GetSizes()[3] == "1920x1080");
                Y_ASSERT(firstItem.GetThumbnail().GetSizes()[4] == "orig");
                Y_ASSERT(firstItem.GetHosting() == "youtube.com");
                Y_ASSERT(firstItem.GetEmbedUri() == "https://www.youtube.com/embed/BtweaJFeqLc?autoplay=1&amp;enablejsapi=1&amp;wmode=opaque");
                Y_ASSERT(firstItem.GetDuration() == 3828);
                Y_ASSERT(firstItem.GetReqId() == "1650322612888870-17916904206322976591-sas3-0806-305-sas-l7-balancer-8080-BAL-4977");
                Y_ASSERT(firstItem.GetReleaseDate() == 1615964400);

                const TWebVideoItem& secondItem = webItems[1].GetSearchVideoItem();
                Y_ASSERT(secondItem.GetTitle() == "Помощь Друзьям - Мультфильм про машинки – Котенок и волшебный гараж – Для самых маленьких");
                Y_ASSERT(secondItem.GetEmbedUri() == "https://www.youtube.com/embed/c1NvsaTkY5I?autoplay=1&amp;enablejsapi=1&amp;wmode=opaque");
                Y_ASSERT(secondItem.GetDuration() == 4610);
                Y_ASSERT(secondItem.GetReleaseDate() == 1629010800);
            }

            Y_UNIT_TEST(ParseBadClips) {
                const TString oneClipOnly = R"(
                {
                    "clips": [
                        {
                            "is_avod": null,
                            "previews": [
                                {
                                    "type": "video/mp4",
                                    "url": "https://video-preview.s3.yandex.net/KSQJegEAAAA.mp4"
                                },
                                {
                                    "ishighres": true,
                                    "type": "video/mp4",
                                    "url": "https://video-preview.s3.yandex.net/hr/_ngJAgAAAAA.mp4",
                                    "width": 1280,
                                    "height": 720
                                }
                            ],
                            "VisibleHost": "youtube.com",
                            "players": {
                                "autoplay": {
                                    "html": "<iframe src=\"//www.youtube.com/embed/BtweaJFeqLc?autoplay=1&amp;enablejsapi=1&amp;wmode=opaque\" frameborder=\"0\" scrolling=\"no\" allowfullscreen=\"1\" allow=\"autoplay; fullscreen; accelerometer; gyroscope; picture-in-picture\" aria-label=\"Video\"></iframe>"
                                },
                                "noautoplay": {
                                    "html": "<iframe src=\"//www.youtube.com/embed/BtweaJFeqLc?enablejsapi=1&amp;wmode=opaque\" frameborder=\"0\" scrolling=\"no\" allowfullscreen=\"1\" allow=\"autoplay; fullscreen; accelerometer; gyroscope; picture-in-picture\" aria-label=\"Video\"></iframe>"
                                }
                            },
                            "factors": {
                                "IsPornoDoc": null
                            },
                            "title": "Маша и Медведь - 💥 Новая Серия! 🐻 Живая шляпа 🎃 Коллекция мультиков для детей про Машу",
                            "PlayerId": "youtube",
                            "thmb_href": "//avatars.mds.yandex.net/get-vthumb/774126/8136da055769db874d1c5f1ef57363c8",
                            "mtime": "June, 5, 2022",
                            "onto_id": null,
                            "thumbs": {
                                "regular": {
                                    "h": "68",
                                    "w": 120,
                                    "url": "//avatars.mds.yandex.net/get-vthumb/774126/8136da055769db874d1c5f1ef57363c8"
                                },
                                "large": {
                                    "h": 180,
                                    "w": 320,
                                    "url": "//avatars.mds.yandex.net/get-vthumb/774126/8136da055769db874d1c5f1ef57363c8"
                                }
                            },
                            "vh_uuid": null,
                            "thumb": "i.ytimg.com/vi/BtweaJFeqLc/0.jpg",
                            "pass": "Премьера! 🔥🍒 Калинка-Малинка 🍓 https://youtu.be/Lzl-515yo9M Песенки для малышей 👶 Маша и Медведь 👧🐻 https://bit.ly/367gOit #промашу #мультик #машаимедведь В сборнике: 00:00 Живая шляпа 🎩...",
                            "duration": 3828,
                            "is_svod": null,
                            "url": "http://www.youtube.com/watch?v=BtweaJFeqLc"
                        },
                    ]
                })";
                const NJson::TJsonValue& document = JsonFromString(oneClipOnly);

                NProtoBuf::RepeatedPtrField<NTv::TCarouselItemWrapper>  webItems = SearchResultParser::ParseClips(document, TRTLogger::NullLogger());

                // in json doc field 'mtime' is not in Iso format, check it was parsed without exception
                Y_ASSERT(webItems.size() == 1);

                const TWebVideoItem& item = webItems[0].GetSearchVideoItem();
                Y_ASSERT(item.GetId() == "BtweaJFeqLc");
                Y_ASSERT(item.GetContentType() == "video");
                Y_ASSERT(item.GetTitle() == "Маша и Медведь - 💥 Новая Серия! 🐻 Живая шляпа 🎃 Коллекция мультиков для детей про Машу");
                Y_ASSERT(item.GetReleaseDate() == 0);
            }

            Y_UNIT_TEST(ParseRelated) {
                const NJson::TJsonValue& document = getFixture("fixtures/game_of_thrones.json");

                TMaybe<NProtoBuf::RepeatedPtrField<NTv::TCarousel>> carouselsMaybe = SearchResultParser::ParseRelatedObjects(document["entity_data"], TRTLogger::NullLogger());
                Y_ASSERT(carouselsMaybe.Defined());
                NProtoBuf::RepeatedPtrField<NTv::TCarousel> carousels = std::move(*carouselsMaybe);
                Y_ASSERT(carousels.size() == 3);

                {
                    NTv::TCarousel carousel = carousels[0];
                    Y_ASSERT(carousel.ItemsSize() == 10);
                    Y_ASSERT(carousel.GetTitle() == "Актёры");
                    Y_ASSERT(carousel.GetId() == "ruw1943831:team");
                    Y_ASSERT(carousel.GetItems(0).HasPersonItem());
                    Y_ASSERT(!carousel.GetItems(0).HasCollectionItem());
                    Y_ASSERT(!carousel.GetItems(0).HasVideoItem());
                    Y_ASSERT(!carousel.GetItems(0).HasSearchVideoItem());


                    auto person = carousel.GetItems(0).GetPersonItem();
                    Y_ASSERT(person.GetKpId() == "1906");
                    Y_ASSERT(person.GetName() == "Питер Динклэйдж");
                    Y_ASSERT(person.GetDescription() == "Американский актёр и кинопродюсер. Начал карьеру в 1995 году; широкая известность пришла после роли в фильме 2003 года «Станционный смотритель», за которую был номинирован на премию Гильдии киноактёров США 2004 года.");
                    Y_ASSERT(person.GetSubtitle() == "Американский актёр");
                    Y_ASSERT(person.GetEntref() == "0oCgpydXcyOTU3MzYyEg9ydXcxOTQzODMxOnRlYW0YAnoM0JDQutGC0ZHRgNGLe9DjMA");
                    Y_ASSERT(person.GetSearchQuery() == "Питер Динклэйдж");
                    Y_ASSERT(person.GetImage().GetBaseUrl() == "https://avatars.mds.yandex.net/get-kinopoisk-image/4483445/e5c28ceb-a020-4474-8c91-5e2e3cb4cd44/");
                    Y_ASSERT(person.GetImage().GetSizes().size() == 5);
                    Y_ASSERT(person.GetImage().GetSizes()[0] == "120x90");
                    Y_ASSERT(person.GetImage().GetSizes()[1] == "400x300");
                    Y_ASSERT(person.GetImage().GetSizes()[2] == "360x540");
                    Y_ASSERT(person.GetImage().GetSizes()[3] == "1920x1080");
                    Y_ASSERT(person.GetImage().GetSizes()[4] == "orig");
                }
                {
                    NTv::TCarousel carousel = carousels[1];
                    Y_ASSERT(carousel.ItemsSize() == 13); // seven invalid is eated =)
                    Y_ASSERT(carousel.GetTitle() == "Смотрите также");
                    Y_ASSERT(carousel.GetId() == "ruw1943831:assoc");
                    Y_ASSERT(carousel.GetItems(0).HasVideoItem());
                    Y_ASSERT(!carousel.GetItems(0).HasPersonItem());
                    Y_ASSERT(!carousel.GetItems(0).HasCollectionItem());
                    Y_ASSERT(!carousel.GetItems(0).HasSearchVideoItem());

                    {
                        auto video = carousel.GetItems(0).GetVideoItem();
                        Y_ASSERT(video.GetProviderItemId() == "4b255f66e459495999b19aaa5825ed83");
                        Y_ASSERT(video.GetMiscIds().GetOntoId() == "ruw1972060");
                        Y_ASSERT(video.GetContentType() == "tv_show");
                        Y_ASSERT(video.GetTitle() == "Тюдоры");
                        Y_ASSERT(video.GetDescription() == "Публичная и тайная жизнь представителей династии Тюдоров — противоречивого периода истории Англии. Процветание и разорение, мудрость королей и деспотия тиранов, скрытые аспекты жизни величайших деятелей того времени.");
                        Y_ASSERT(video.GetPoster().GetBaseUrl() == "https://avatars.mds.yandex.net/get-kinopoisk-image/1629390/e629aae0-e232-4a87-8976-77c4c1752eb7/");
                        Y_ASSERT(video.GetPoster().GetSizes().size() == 5);
                        Y_ASSERT(video.GetPoster().GetSizes()[0] == "120x90");
                        Y_ASSERT(video.GetPoster().GetSizes()[1] == "400x300");
                        Y_ASSERT(video.GetPoster().GetSizes()[2] == "360x540");
                        Y_ASSERT(video.GetPoster().GetSizes()[3] == "1920x1080");
                        Y_ASSERT(video.GetPoster().GetSizes()[4] == "orig");
                        Y_ASSERT(video.GetEntref() == "0oCgpydXcxOTcyMDYwEhBydXcxOTQzODMxOmFzc29jGAJ6G9Ch0LzQvtGC0YDQuNGC0LUg0YLQsNC60LbQtZ0BjPM");
                        Y_ASSERT(video.GetAgeLimit() == "16");
                        Y_ASSERT(video.GetSearchQuery() == "тюдоры сериал");
                        Y_ASSERT(video.GetVhLicences().GetAvod() == 0);
                        Y_ASSERT(video.GetVhLicences().GetContentType() == "TV_SERIES");
                        Y_ASSERT(video.GetRating() == 8);
                        Y_ASSERT(video.GetReleaseDate() == 2007);
                        Y_ASSERT(video.GetHintDescription() == "2007–2010, драма, мелодрама");
                    }
                    {
                        auto video = carousel.GetItems(1).GetVideoItem();
                        Y_ASSERT(video.GetProviderItemId() == "42de120c6f62717f922bb76ba02b12ea");
                        Y_ASSERT(video.GetMiscIds().GetOntoId() == "ruw4473466");
                        Y_ASSERT(video.GetContentType() == "tv_show");
                        Y_ASSERT(video.GetTitle() == "Демоны Да Винчи");
                        Y_ASSERT(video.GetDescription() == "В мире, где мысль и вера находятся под контролем, один человек борется за то, чтобы сделать знания доступными для всех. Не рассказанная история трагичной жизни Леонардо Да Винчи раскрывает портрет молодого человека, которого мучает дар гения. Он - еретик, жаждущий раскрыть ложь религии. Бунтарь, стремящийся низвергнуть элитарное общество. Незаконнорожденный сын, требующий от своего отца признания его прав. Он оказывается посреди разразившегося урагана страстей, который спал веками. Внутри конфликта между правдой и ложью, религией и интеллектом, прошлым и будущим...");
                        Y_ASSERT(video.GetPoster().GetBaseUrl() == "https://avatars.mds.yandex.net/get-kinopoisk-image/1704946/dfef323a-cfdb-42c4-b497-71c61872a8d6/");
                        Y_ASSERT(video.GetPoster().GetSizes().size() == 5);
                        Y_ASSERT(video.GetPoster().GetSizes()[0] == "120x90");
                        Y_ASSERT(video.GetPoster().GetSizes()[1] == "400x300");
                        Y_ASSERT(video.GetPoster().GetSizes()[2] == "360x540");
                        Y_ASSERT(video.GetPoster().GetSizes()[3] == "1920x1080");
                        Y_ASSERT(video.GetPoster().GetSizes()[4] == "orig");
                        Y_ASSERT(video.GetEntref() == "0oCgpydXc0NDczNDY2EhBydXcxOTQzODMxOmFzc29jGAJ6G9Ch0LzQvtGC0YDQuNGC0LUg0YLQsNC60LbQtSZvMzM");
                        Y_ASSERT(video.GetAgeLimit() == "18");
                        Y_ASSERT(video.GetSearchQuery() == "демоны да винчи сериал");
                        Y_ASSERT(video.GetVhLicences().SvodSize() == 6);
                        Y_ASSERT(video.GetVhLicences().GetSvod()[0] == "YA_PLUS");
                        Y_ASSERT(video.GetVhLicences().GetSvod()[1] == "YA_PLUS_SUPER");
                        Y_ASSERT(video.GetVhLicences().GetSvod()[2] == "YA_PLUS_3M");
                        Y_ASSERT(video.GetVhLicences().GetSvod()[3] == "YA_PREMIUM");
                        Y_ASSERT(video.GetVhLicences().GetSvod()[4] == "KP_BASIC");
                        Y_ASSERT(video.GetVhLicences().GetSvod()[5] == "YA_PLUS_KP");
                        Y_ASSERT(video.GetVhLicences().GetContentType() == "TV_SERIES");
                        Y_ASSERT(video.GetRating() == 7.75);
                        Y_ASSERT(video.GetReleaseDate() == 2013);
                        Y_ASSERT(video.GetHintDescription() == "2013–2015, фэнтези, драма");
                    }
                }
                {
                    NTv::TCarousel carousel = carousels[2];
                    Y_ASSERT(carousel.ItemsSize() == 6);
                    Y_ASSERT(carousel.GetTitle() == "Похожие подборки");
                    Y_ASSERT(carousel.GetId() == "ruw1943831:collections");
                    Y_ASSERT(carousel.GetItems(0).HasCollectionItem());
                    Y_ASSERT(!carousel.GetItems(0).HasVideoItem());
                    Y_ASSERT(!carousel.GetItems(0).HasPersonItem());
                    Y_ASSERT(!carousel.GetItems(0).HasSearchVideoItem());

                    {
                        auto video = carousel.GetItems(0).GetCollectionItem();
                        Y_ASSERT(video.GetId() == "lst-55829456..0");
                        Y_ASSERT(video.GetEntref() == "0oEg9sc3QtNTU4Mjk0NTYuLjAYAnof0J_QvtGF0L7QttC40LUg0L_QvtC00LHQvtGA0LrQuNaFnGs");
                        Y_ASSERT(video.GetTitle() == "Сериалы с субтитрами");
                        Y_ASSERT(video.GetSearchQuery() == "сериалы с субтитрами ivi ru");
                        Y_ASSERT(video.GetImages().size() == 3);
                        Y_ASSERT(video.GetImages().Get(0).GetBaseUrl() == "https://avatars.mds.yandex.net/get-kinopoisk-image/1946459/9f91eb22-2848-48c8-92e9-5ff219e153da/");
                        Y_ASSERT(video.GetImages().Get(1).GetBaseUrl() == "https://avatars.mds.yandex.net/get-kinopoisk-image/4303601/db514b79-0174-4acd-a432-87a8c97cdff1/");
                        Y_ASSERT(video.GetImages().Get(2).GetBaseUrl() == "https://avatars.mds.yandex.net/get-kinopoisk-image/1599028/1f28b993-18d8-4a53-bb6c-b31ff613efc3/");

                        for (const auto& image : video.GetImages()) {
                            Y_ASSERT(image.GetSizes().size() == 5);
                            Y_ASSERT(image.GetSizes()[0] == "160x90");
                            Y_ASSERT(image.GetSizes()[1] == "720x360");
                            Y_ASSERT(image.GetSizes()[2] == "960x540");
                            Y_ASSERT(image.GetSizes()[3] == "1920x1080");
                            Y_ASSERT(image.GetSizes()[4] == "orig");
                        }
                    }
                }
            }

            Y_UNIT_TEST(GetMdsUrlInfo) {
                const std::vector<const std::pair<const TString, const TString>> testData{
                        {"//avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/orig", "https://avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/"},
                        {"//avatars.mds.yandex.net:80/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/orig", "https://avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/"},
                        {"//avatars.mds.yandex.net:443/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/orig", "https://avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/"},
                        {"https://avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/orig", "https://avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/"},
                        {"https://avatars.mds.yandex.net:80/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/orig", "https://avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/"},
                        {"https://avatars.mds.yandex.net:443/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/orig", "https://avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/"},
                        {"http://avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/orig", "https://avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/"},
                        {"http://avatars.mds.yandex.net:80/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/orig", "https://avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/"},
                        {"http://avatars.mds.yandex.net:443/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/orig", "https://avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/"},

                        {"//avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/orig", "https://avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/"},
                        {"//avatars.mds.yandex.net:80/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/orig", "https://avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/"},
                        {"//avatars.mds.yandex.net:443/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/orig", "https://avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/"},
                        {"https://avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/orig", "https://avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/"},
                        {"https://avatars.mds.yandex.net:80/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/orig", "https://avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/"},
                        {"https://avatars.mds.yandex.net:443/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/orig", "https://avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/"},
                        {"http://avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/orig", "https://avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/"},
                        {"http://avatars.mds.yandex.net:80/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/orig", "https://avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/"},
                        {"http://avatars.mds.yandex.net:443/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/orig", "https://avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/"},

                        {"//avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/150x180", "https://avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/"},
                        {"//avatars.mds.yandex.net:80/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/150x180", "https://avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/"},
                        {"//avatars.mds.yandex.net:443/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/150x180", "https://avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/"},
                        {"https://avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/150x180", "https://avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/"},
                        {"https://avatars.mds.yandex.net:80/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/150x180", "https://avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/"},
                        {"https://avatars.mds.yandex.net:443/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/150x180", "https://avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/"},
                        {"http://avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/150x180", "https://avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/"},
                        {"http://avatars.mds.yandex.net:80/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/150x180", "https://avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/"},
                        {"http://avatars.mds.yandex.net:443/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/150x180", "https://avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/"},

                        {"//avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/150x180_1", "https://avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/"},
                        {"//avatars.mds.yandex.net:80/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/150x180_1", "https://avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/"},
                        {"//avatars.mds.yandex.net:443/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/150x180_1", "https://avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/"},
                        {"https://avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/150x180_1", "https://avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/"},
                        {"https://avatars.mds.yandex.net:80/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/150x180_1", "https://avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/"},
                        {"https://avatars.mds.yandex.net:443/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/150x180_1", "https://avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/"},
                        {"http://avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/150x180_1", "https://avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/"},
                        {"http://avatars.mds.yandex.net:80/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/150x180_1", "https://avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/"},
                        {"http://avatars.mds.yandex.net:443/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/150x180_1", "https://avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/"},
                        {"https://avatars.mds.yandex.net/get-kinopoisk-image/1600647/f06aa25d-2f76-4bac-8d12-29436fd589bc/orig", "https://avatars.mds.yandex.net/get-kinopoisk-image/1600647/f06aa25d-2f76-4bac-8d12-29436fd589bc/"},
                        {"https://avatars.mds.yandex.net/get-kinopoisk-image/1600647/f06aa25d-2f76-4bac-8d12-29436fd589bc/", "https://avatars.mds.yandex.net/get-kinopoisk-image/1600647/f06aa25d-2f76-4bac-8d12-29436fd589bc/"},
                        {"https://avatars.mds.yandex.net:80/get-kinopoisk-image/1600647/f06aa25d-2f76-4bac-8d12-29436fd589bc/orig", "https://avatars.mds.yandex.net/get-kinopoisk-image/1600647/f06aa25d-2f76-4bac-8d12-29436fd589bc/"},
                };
                for (const auto& pair: testData) {
                    const TMaybe<TString>& actualResult = GetMdsUrlInfo(pair.first);

                    UNIT_ASSERT(actualResult.Defined());
                    UNIT_ASSERT(actualResult.GetRef() == pair.second);
                }
            }

            Y_UNIT_TEST(BuildPosterImageUrl) {
                TVector<TString> params{"3х4", "orig"};
                const TMaybe<TAvatarMdsImage>& maybeImage = BuildImage(
                        "//avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/orig",
                        params
                );
                UNIT_ASSERT(maybeImage.Defined());
                UNIT_ASSERT(maybeImage->GetBaseUrl() == "https://avatars.mds.yandex.net/get-ott/239697/2a000001612ca8b8edcc1e4f560a2ee18de2/");

                UNIT_ASSERT(maybeImage->GetSizes().size() == 2);
                UNIT_ASSERT(maybeImage->GetSizes()[0] == "3х4");
                UNIT_ASSERT(maybeImage->GetSizes()[1] == "orig");
            }

            Y_UNIT_TEST(ParseParentCollectionWithOffers) {
                const NJson::TJsonValue& document = getFixture("fixtures/vh_exp_film_offers.json");

                const auto maybeCarousel = SearchResultParser::ParseParentCollectionObjects(document["entity_data"], TRTLogger::NullLogger());
                UNIT_ASSERT(maybeCarousel.Defined());

                UNIT_ASSERT_EQUAL(maybeCarousel->ItemsSize(), 4);
                for (size_t i = 0; i < maybeCarousel->ItemsSize(); ++i) {
                    UNIT_ASSERT(maybeCarousel->GetItems(i).HasVideoItem());
                }

                auto videoItem0 = maybeCarousel->GetItems(0).GetVideoItem();
                UNIT_ASSERT_EQUAL(videoItem0.GetTitle(), "Терминатор 3: Восстание машин");
                UNIT_ASSERT_EQUAL(videoItem0.GetReleaseDate(), 2003);
                UNIT_ASSERT_EQUAL(videoItem0.CinemasSize(), 5);

                auto videoItem1 = maybeCarousel->GetItems(1).GetVideoItem();
                UNIT_ASSERT_EQUAL(videoItem1.GetTitle(), "Терминатор: Да придёт спаситель");
                UNIT_ASSERT_EQUAL(videoItem1.GetAgeLimit(), "16");
                UNIT_ASSERT_EQUAL(videoItem1.CinemasSize(), 6);

                auto cinemas = videoItem1.GetCinemas();
                UNIT_ASSERT_EQUAL(cinemas.size(), 6);
                {
                    auto cinema = cinemas.Get(0);
                    UNIT_ASSERT_EQUAL(cinema.GetCinemaName(), "Кино1ТВ");
                    UNIT_ASSERT_EQUAL(cinema.VariantsSize(), 1);
                    UNIT_ASSERT_EQUAL(cinema.GetVariants(0).GetPrice(), 599);
                    UNIT_ASSERT_EQUAL(cinema.GetVariants(0).GetType(), "svod");
                }
                {
                    auto cinema = cinemas.Get(1);
                    UNIT_ASSERT_EQUAL(cinema.GetCinemaName(), "IVI");
                    UNIT_ASSERT_EQUAL(cinema.VariantsSize(), 2);
                    UNIT_ASSERT_EQUAL(cinema.GetVariants(0).GetPrice(), 399);
                    UNIT_ASSERT_EQUAL(cinema.GetVariants(0).GetType(), "est");
                    UNIT_ASSERT_EQUAL(cinema.GetVariants(1).GetPrice(), 599);
                    UNIT_ASSERT_EQUAL(cinema.GetVariants(1).GetType(), "svod");
                }
                {
                    auto cinema = cinemas.Get(2);
                    UNIT_ASSERT_EQUAL(cinema.GetCinemaName(), "KION");
                }
                {
                    auto cinema = cinemas.Get(3);
                    UNIT_ASSERT_EQUAL(cinema.GetCinemaName(), "Okko");
                }
                {
                    auto cinema = cinemas.Get(4);
                    UNIT_ASSERT_EQUAL(cinema.GetCinemaName(), "START");
                }
                {
                    auto cinema = cinemas.Get(5);
                    UNIT_ASSERT_EQUAL(cinema.GetCinemaName(), "Wink");
                    UNIT_ASSERT_EQUAL(cinema.VariantsSize(), 3);
                }
            }

            Y_UNIT_TEST(ParseParentCollectionGarfild) {
                const NJson::TJsonValue& document = getFixture("fixtures/garfild.json");

                const auto maybeCarousel = SearchResultParser::ParseParentCollectionObjects(document["entity_data"], TRTLogger::NullLogger());
                UNIT_ASSERT(maybeCarousel.Defined());

                UNIT_ASSERT_EQUAL(maybeCarousel->ItemsSize(), 2);
                for (size_t i = 0; i < maybeCarousel->ItemsSize(); ++i) {
                    const auto item = maybeCarousel->GetItems(i).GetVideoItem();
                    UNIT_ASSERT(item.HasPoster());
                    UNIT_ASSERT((item.HasVhLicences() && !item.GetProviderItemId().Empty()) 
                              || item.CinemasSize() > 0);
                }
            }

            Y_UNIT_TEST(ParseTerminator2RelatedObjects) {
                const NJson::TJsonValue& document = getFixture("fixtures/terminator_2.json");
                auto carousels = SearchResultParser::ParseRelatedObjects(document["entity_data"], TRTLogger::NullLogger());
                UNIT_ASSERT(carousels.Defined());
                const auto assocIt = std::find_if(carousels->begin(), carousels->end(), [](const NTv::TCarousel& carousel) {
                    return (carousel.GetId() == "ruw98189:assoc");
                });
                UNIT_ASSERT(assocIt != carousels->end());
                NTv::TCarousel assocItems = *assocIt;
                for (size_t i = 0; i < assocItems.ItemsSize(); ++i) {
                    const auto videoItem = assocItems.GetItems(i).GetVideoItem();
                    const auto vhLicenses = videoItem.GetVhLicences();
                    UNIT_ASSERT(vhLicenses.GetContentType() != "KP_TRAILER" && vhLicenses.GetContentType() != "TRAILER");
                }
            }
    }

    Y_UNIT_TEST_SUITE(ParseWebSearchData) {
        Y_UNIT_TEST(ParseProtoSmychok) {
            TFileInput inputData("fixtures/Smychok.json");
            TString snippetString = inputData.ReadAll();

            NJson::TJsonValue respJson = JsonFromString(snippetString);

            respJson["entity_data"] = respJson["data"];
            TTvSearchResultData searchResultFromJson = SearchResultParser::ParseJsonResponse(respJson, TRTLogger::NullLogger());

            google::protobuf::Struct entitySnippet;
            google::protobuf::util::JsonParseOptions options;
            google::protobuf::util::JsonStringToMessage(snippetString, &entitySnippet, options);

            TProtoAdapter entitySnippetAdapter(entitySnippet);
            // baseInfo["type"] == Film, but baseInfo["legal"]["vh_licences"]["content_type"] == "KP_TRAILER" -> не используется 
            // также есть baseInfo["id"] - можно использовать под onto_id полупиратской карточки
//            TMaybe<NTv::TCarouselItemWrapper> maybeBaseInfo = SearchResultParser::ParseBaseInfo(entitySnippetAdapter["data"], TRTLogger::NullLogger());
//            if (maybeBaseInfo.Defined()) {
//                Cout << "Smychock has base info";
//            } 
            TTvSearchResultData searchResult = SearchResultParser::ParseProtoResponse(entitySnippetAdapter, TRTLogger::NullLogger());

            UNIT_ASSERT_STRINGS_EQUAL(searchResultFromJson.DebugString(), searchResult.DebugString());
//            Cout << searchResult.DebugString();
        }

        Y_UNIT_TEST(ParseProtoPrizraki) {
            TFileInput inputData("fixtures/Prizraki.json");
            TString snippetString = inputData.ReadAll();

            google::protobuf::Struct entitySnippet;
            google::protobuf::util::JsonParseOptions options;
            google::protobuf::util::JsonStringToMessage(snippetString, &entitySnippet, options);

            TProtoAdapter entitySnippetAdapter(entitySnippet);
            // baseInfo["type"] == Film, but baseInfo["legal"]["vh_licences"]["content_type"] == "KP_TRAILER" -> не используется 
            // также есть baseInfo["id"] - можно использовать под onto_id полупиратской карточки => все тоже что и со Смычком
//            TMaybe<NTv::TCarouselItemWrapper> maybeBaseInfo = SearchResultParser::ParseBaseInfo(entitySnippetAdapter["data"], TRTLogger::NullLogger());
//            if (maybeBaseInfo.Defined()) {
//                Cout << "Prizraki has base info";
//            } 
            TTvSearchResultData searchResult = SearchResultParser::ParseProtoResponse(entitySnippetAdapter, TRTLogger::NullLogger());
            Cout << searchResult.DebugString();
        }

        Y_UNIT_TEST(ParseProtoFriends) {
            TFileInput inputData("fixtures/Friends.json");
            TString snippetString = inputData.ReadAll();

            google::protobuf::Struct entitySnippet;
            google::protobuf::util::JsonParseOptions options;
            google::protobuf::util::JsonStringToMessage(snippetString, &entitySnippet, options);

            TProtoAdapter entitySnippetAdapter(entitySnippet);
            // baseInfo["type"] == Film, but baseInfo["legal"]["vh_licences"]["content_type"] == "TRAILER" -> не используется 
            // также есть baseInfo["id"] - можно использовать под onto_id полупиратской карточки => все тоже что и со Смычком и с Призраками
            // Друзей нет в кинопоиске, но они есть в сторонних кинотеатрах и выделяются ОО
            TTvSearchResultData searchResult = SearchResultParser::ParseProtoResponse(entitySnippetAdapter, TRTLogger::NullLogger(), true);
            UNIT_ASSERT(searchResult.GalleriesSize() > 0);
            const auto firstItem = searchResult.GetGalleries(0).GetBasicCarousel().GetItems(0);
            UNIT_ASSERT(firstItem.HasVideoItem());
            TOttVideoItem friendsSeries = firstItem.GetVideoItem();
            UNIT_ASSERT_STRINGS_EQUAL(friendsSeries.GetTitle(), "Друзья");
            UNIT_ASSERT(friendsSeries.HasPoster());
            UNIT_ASSERT(friendsSeries.HasVhLicences());
            UNIT_ASSERT(friendsSeries.HasMiscIds());
            UNIT_ASSERT_EQUAL(friendsSeries.GetMiscIds().GetOntoId(), "ruw86047");
            UNIT_ASSERT(friendsSeries.CinemasSize() > 0);
            UNIT_ASSERT_STRINGS_EQUAL(friendsSeries.GetCinemas(0).GetTvDeepLink(), "yotaplay://movie?uid=36a06f68-a0e6-4091-a80d-8fd7aca02913&type=serial&action=play");
        }

        Y_UNIT_TEST(ParseProtoHowIMetYourMother) {
            TFileInput inputData("fixtures/HowIMetYourMother.json");
            TString snippetString = inputData.ReadAll();

            google::protobuf::Struct entitySnippet;
            google::protobuf::util::JsonParseOptions options;
            google::protobuf::util::JsonStringToMessage(snippetString, &entitySnippet, options);

            TProtoAdapter entitySnippetAdapter(entitySnippet);

            // есть id, есть type==Film, но нет блока legal
            // отбрасываем, хотя потенциал есть
            // более того, "Как я встретил вашу маму" доступен в КП
//            const auto maybeBaseInfo = SearchResultParser::ParseBaseInfo(entitySnippetAdapter["data"], TRTLogger::NullLogger());
//            if (maybeBaseInfo.Defined()) {
//                Cout << maybeBaseInfo->DebugString();
//            } else {
//                Cout << "No base info parsed";
//            }
            TTvSearchResultData searchResult = SearchResultParser::ParseProtoResponse(entitySnippetAdapter, TRTLogger::NullLogger());
            Cout << searchResult.DebugString();           
        }

        Y_UNIT_TEST(ParseProtoSupernatural) {
            TFileInput inputData("fixtures/Supernatural.json");
            TString snippetString = inputData.ReadAll();

            google::protobuf::Struct entitySnippet;
            google::protobuf::util::JsonParseOptions options;
            google::protobuf::util::JsonStringToMessage(snippetString, &entitySnippet, options);

            TProtoAdapter entitySnippetAdapter(entitySnippet);

            const clock_t baseInfo_t1 = clock();
            const auto maybeBaseInfo = SearchResultParser::ParseBaseInfo(entitySnippetAdapter["data"], TRTLogger::NullLogger());
            const clock_t baseInfo_t2 = clock();

            UNIT_ASSERT(maybeBaseInfo.Defined());
            const auto ottItem = maybeBaseInfo->GetVideoItem();
            UNIT_ASSERT_STRINGS_EQUAL(ottItem.GetTitle(), "Сверхъестественное");
            UNIT_ASSERT(ottItem.HasMiscIds());
            UNIT_ASSERT(ottItem.HasVhLicences());

            const clock_t searchResult_t1 = clock();
            const auto searchResult = SearchResultParser::ParseProtoResponse(entitySnippetAdapter, TRTLogger::NullLogger());
            const clock_t searchResult_t2 = clock();

            Cout << "BaseInfo parse time: " << (baseInfo_t2 - baseInfo_t1) / (double)CLOCKS_PER_SEC << " sec\n";
            Cout << "SearchResult parse time: " << (searchResult_t2 - searchResult_t1) / (double)CLOCKS_PER_SEC << " sec" << Endl;
        }

        Y_UNIT_TEST(ParseProtoMatrix) {
            TFileInput inputData("fixtures/Matrix.json");
            TString snippetString = inputData.ReadAll();

            google::protobuf::Struct entitySnippet;
            google::protobuf::util::JsonParseOptions options;
            google::protobuf::util::JsonStringToMessage(snippetString, &entitySnippet, options);

            TProtoAdapter entitySnippetAdapter(entitySnippet);

            const auto maybeBaseInfo = SearchResultParser::ParseBaseInfo(entitySnippetAdapter["data"], TRTLogger::NullLogger());

            // There should be parent collection, not base info
            UNIT_ASSERT(!maybeBaseInfo.Defined());

            const auto searchResult = SearchResultParser::ParseProtoResponse(entitySnippetAdapter, TRTLogger::NullLogger());
            // Есть parent_collection, в ней 3 ( :( ) фильма, но тип контента - TRAILER и нет ни cinema_data, ни film_offers - очень плохой выхлоп
            
            Cout << searchResult.DebugString();
        }
    }

    Y_UNIT_TEST_SUITE(MonitoringTest) {

        using namespace NAlice::NHollywoodFw::NVideo::NSearchMetrics;

        Y_UNIT_TEST(TestWebParsersSolomoning) {
  
            using namespace NAlice::NHollywoodFw::NVideo::NSearchMetrics::Monitoring::Web;

            auto& logger = TRTLogger::NullLogger();

            auto checkAnotherSensor = [](const TFakeSensors& sensors, TStringBuf label, TStringBuf value, TMaybe<int> expected) {
                const auto* sens = sensors.FindFirstRateSensor(label, value);
                if (expected.Defined()) {
                    UNIT_ASSERT_C(sens, TStringBuilder() << " : no rate sensor with " << label << " equals " << value);
                    UNIT_ASSERT_EQUAL_C(sens->Value, expected, TStringBuilder() << " : actual value of sensor " << value << " is " << sens->Value);
                } else {
                    UNIT_ASSERT(nullptr == sens);
                }
            };
            auto checkSensor = [&checkAnotherSensor](const TFakeSensors& sensors, TStringBuf name, TMaybe<int> value) {
                return checkAnotherSensor(sensors, "subname", name, value);
            };
            {
                auto sensors = TFakeSensors();
    
                TrackNoSnippet(sensors, logger);
                TrackNoSnippet(sensors, logger);

                TrackNoDatasource(sensors, logger);
                TrackNoDatasource(sensors, logger);
                TrackNoDatasource(sensors, logger);

                TrackNoSnippet(sensors, logger);
                TrackNoSnippet(sensors, logger);

                checkSensor(sensors, "no_snippet", 4);
                checkSensor(sensors, "no_datasource", 3);
            }
            {
                auto sensors = TFakeSensors();

                TFileInput inputData("fixtures/Supernatural.json");
                TString snippetString = inputData.ReadAll();
    
                google::protobuf::Struct snippet;
                google::protobuf::util::JsonParseOptions options;
                google::protobuf::util::JsonStringToMessage(snippetString, &snippet, options);

                clock_t t0 = clock();
                TrackWebSnippet(sensors, logger, snippet);
                clock_t t1 = clock();
                Cout << "example #1 track time: " << (t1 - t0) / (double)CLOCKS_PER_SEC << " sec\n";

                checkSensor(sensors, "no_snippet", Nothing());
                checkSensor(sensors, "no_datasource", Nothing());
                checkSensor(sensors, "has_onto_id", 1);
                checkAnotherSensor(sensors, "license", "other", 1);
                checkSensor(sensors, "has_base_info", 1);
                checkSensor(sensors, "trailer_content_type", Nothing());
            }
            {
                auto sensors = TFakeSensors();

                TFileInput inputData("fixtures/HowIMetYourMother.json");
                TString snippetString = inputData.ReadAll();
    
                google::protobuf::Struct snippet;
                google::protobuf::util::JsonParseOptions options;
                google::protobuf::util::JsonStringToMessage(snippetString, &snippet, options);

                clock_t t0 = clock();
                TrackWebSnippet(sensors, logger, snippet);
                clock_t t1 = clock();
                Cout << "example #2 track time: " << (t1 - t0) / (double)CLOCKS_PER_SEC << " sec\n";

                checkSensor(sensors, "no_snippet", Nothing());
                checkSensor(sensors, "no_datasource", Nothing());
                checkSensor(sensors, "has_base_info", 1);
                checkSensor(sensors, "has_onto_id", 1);
                checkAnotherSensor(sensors, "license", "no_license", 1);
                checkAnotherSensor(sensors, "license", "trailer", Nothing());
                checkAnotherSensor(sensors, "license", "other", Nothing());
                checkSensor(sensors, "has_related_object", 1);
                checkSensor(sensors, "has_parent_collection", Nothing());
            }
            {
                auto sensors = TFakeSensors();

                TFileInput inputData("fixtures/Smychok.json");
                TString snippetString = inputData.ReadAll();
    
                google::protobuf::Struct snippet;
                google::protobuf::util::JsonParseOptions options;
                google::protobuf::util::JsonStringToMessage(snippetString, &snippet, options);
                
                TrackWebSnippet(sensors, logger, snippet);
 
                checkSensor(sensors, "no_snippet", Nothing());
                checkSensor(sensors, "no_datasource", Nothing());
                checkSensor(sensors, "has_base_info", 1);
                checkSensor(sensors, "has_onto_id", 1);
                checkAnotherSensor(sensors, "license", "no_license", Nothing());
                checkAnotherSensor(sensors, "license", "trailer", 1);
                checkAnotherSensor(sensors, "license", "other", Nothing());
            }
            {
                auto sensors = TFakeSensors();
                
                TrackWhichSearchResultWasUsed(sensors, ESearchResultSource::WebSearchBaseInfo);
                TrackWhichSearchResultWasUsed(sensors, ESearchResultSource::VideoSearchBaseInfo);
                TrackWhichSearchResultWasUsed(sensors, ESearchResultSource::VideoSearchAll);
                TrackWhichSearchResultWasUsed(sensors, ESearchResultSource::VideoSearchAll);

                auto checkSourceSensor = [&checkAnotherSensor](const TFakeSensors& sensors, TStringBuf name, TMaybe<int> value) {
                    return checkAnotherSensor(sensors, "source", name, value);
                };

                checkSourceSensor(sensors, "web_base_info", 1);
                checkSourceSensor(sensors, "vs_base_info", 1);
                checkSourceSensor(sensors, "vs_all", 2);
                checkSourceSensor(sensors, "web_search_all", Nothing());
            }
            {
                auto sensors = TFakeSensors();

                TFileInput inputData("fixtures/Matrix.json");
                TString snippetString = inputData.ReadAll();
    
                google::protobuf::Struct snippet;
                google::protobuf::util::JsonParseOptions options;
                google::protobuf::util::JsonStringToMessage(snippetString, &snippet, options);

                TrackWebSnippet(sensors, logger, snippet);

                checkSensor(sensors, "no_snippet", Nothing());
                checkSensor(sensors, "no_datasource", Nothing());
                checkSensor(sensors, "has_base_info", Nothing());
                checkSensor(sensors, "has_related_object", Nothing());
                checkSensor(sensors, "has_parent_collection", 1);
            }
        }

        Y_UNIT_TEST(TestVideoParsersSolomoning) {
  
            auto checkSensor = [](const TFakeSensors& sensors, TStringBuf label, TStringBuf value, TMaybe<int> expected) {
                const auto* sens = sensors.FindFirstRateSensor(label, value);
                if (expected.Defined()) {
                    UNIT_ASSERT_C(sens, TStringBuilder() << " : no rate sensor with " << label << " equals " << value);
                    UNIT_ASSERT_EQUAL_C(sens->Value, expected, TStringBuilder() << " : actual value of sensor " << value << " is " << sens->Value);
                } else {
                    UNIT_ASSERT(nullptr == sens);
                }
            };
            {
                auto sensors = TFakeSensors();

                TrackVideoSearchResponded(sensors, true);
                TrackVideoSearchResponded(sensors, true);
                TrackVideoSearchResponded(sensors, true);
                TrackVideoSearchResponded(sensors, false);

                checkSensor(sensors, "success", "true", 3);
                checkSensor(sensors, "success", "false", 1);
            }
            {
                auto sensors = TFakeSensors();

                const NJson::TJsonValue& document = getFixture("fixtures/game_of_thrones.json");

                const auto result = SearchResultParser::ParseJsonResponse(document, TRTLogger::NullLogger(), true);

                TrackVideoSearchResult(sensors, result);

                checkSensor(sensors, "has_gallery", "Актёры", 1);
                checkSensor(sensors, "has_gallery", "Похожие подборки", 1);
            }
        }
    }
}
