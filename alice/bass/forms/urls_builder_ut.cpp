#include <alice/bass/forms/urls_builder.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NBASS;

namespace {

constexpr TStringBuf HTTPS_SCHEMA = "https:";

constexpr TStringBuf QUASAR_APPID = "ru.yandex.quasar.app";
constexpr TStringBuf SEARCHAPP_APPID = "ru.yandex.searchplugin/7.70 (samsung SM-A730F; android 7.1.1)";
constexpr TStringBuf UNKNOWN_APPID = "unknown/1.23 (linux notebook; linux 2.4)";
constexpr TStringBuf YABRO_APPID = "com.yandex.browser/18.7.1.575 (Sony F3111; android 7.0)";

Y_UNIT_TEST_SUITE(UrlsBuilder) {
    Y_UNIT_TEST(News) {
        // Add missing schema. If app isn't search app remove mobile prefix.
        auto testMobileUrl = [](const TStringBuf rawUrl, const TStringBuf resultUrl) {
            const TClientInfo ci(SEARCHAPP_APPID);
            const TString url = GenerateNewsUri(ci, rawUrl);
            UNIT_ASSERT_VALUES_EQUAL(url, resultUrl);
        };
        auto testDesktopUrl = [](const TStringBuf rawUrl, const TStringBuf resultUrl) {
            for (const TStringBuf appId : {QUASAR_APPID, UNKNOWN_APPID, YABRO_APPID}) {
                const TClientInfo ci(appId);
                const TString url = GenerateNewsUri(ci, rawUrl);
                UNIT_ASSERT_VALUES_EQUAL(url, resultUrl);
            }
        };
        // Story url. Expect no difference.
        constexpr TStringBuf STORY_PUTIN_URL =
                        "https://m.news.yandex.ru/yandsearch?cl4url=rg.ru/2017/11/21/"
                        "vladimir-putin-voennaia-operaciia-v-sirii-zavershaetsia.html&lang=ru&from=main_portal&stid="
                        "k1rysBeQHOwu7Lo3_D0y&lr=51";
        testMobileUrl(STORY_PUTIN_URL, STORY_PUTIN_URL);

        constexpr TStringBuf STORY_API_URL =
            "https://yandex.ru/news/story/"
            "Operatory_predupredili_o_roste_tarifov_na_svyaz--ed71bce18672f138eb180ddea09e5b06?lang=ru&from=api-"
            "rubric&rubric=koronavirus&stid=rc-SqzUxu2n8Ri_xVHD5&t=1586852459&tt=true&persistent_id=94023226";
        testDesktopUrl(STORY_API_URL, STORY_API_URL);

        // News wizard return url without schema. =
        constexpr TStringBuf SEARCH_TOUCH_URL =
                        "//m.news.yandex.ru/"
                        "yandsearch?text=%D0%B0%D0%B2%D1%81%D1%82%D1%80%D0%B0%D0%BB%D0%"
                        "B8%D1%8F&rpt=nnews2&rel=rel&grhow=clutop&from=newswizard";

        // For searchpp expect schema is added.
        testMobileUrl(SEARCH_TOUCH_URL, TString::Join(HTTPS_SCHEMA, SEARCH_TOUCH_URL));
        // For others expect valid schema and 'm.' removed.
        constexpr TStringBuf SEARCH_DESKTOP_URL =
                        "https://news.yandex.ru/"
                        "yandsearch?text=%D0%B0%D0%B2%D1%81%D1%82%D1%80%D0%B0%D0%BB%D0%"
                        "B8%D1%8F&rpt=nnews2&rel=rel&grhow=clutop&from=newswizard";
        testDesktopUrl(SEARCH_TOUCH_URL, SEARCH_DESKTOP_URL);

        // Expect rubric url is untouched.
        constexpr TStringBuf RUBRIC_API_URL = "https://news.yandex.ru/sport.html";
        testMobileUrl(RUBRIC_API_URL, RUBRIC_API_URL);
        testDesktopUrl(RUBRIC_API_URL, RUBRIC_API_URL);

        // Expect 'http' is not replaced with 'https'.
        constexpr TStringBuf HTTP_RUBRIC_API_URL = "http://news.yandex.ru/sport.html";
        testMobileUrl(HTTP_RUBRIC_API_URL, HTTP_RUBRIC_API_URL);
    }

    Y_UNIT_TEST(GenerateClientActionUriSmoke) {
        NSc::TValue payload;
        payload["open_fullscreen"].SetString("true");

        UNIT_ASSERT_VALUES_EQUAL(TClientActionUrl({}).ToString(), "dialog-action://");
        UNIT_ASSERT_VALUES_EQUAL(TClientActionUrl::ToString({}), "dialog-action://");
        UNIT_ASSERT_VALUES_EQUAL(TClientActionUrl(TClientActionUrl::EType::ShowTimers).ToString(),
                                 "dialog-action://"
                                 "?directives=%5B%7B%22name%22%3A%22show_timers%22%2C%22type%22%3A%22client_action%22%"
                                 "7D%5D");
        UNIT_ASSERT_VALUES_EQUAL(TClientActionUrl::ToString({TClientActionUrl(TClientActionUrl::EType::ShowTimers), TClientActionUrl(TClientActionUrl::EType::ShowAlarms)}),
                                 "dialog-action://"
                                 "?directives=%5B%7B%22name%22%3A%22show_timers%22%2C%22type%22%3A%22client_action%22%"
                                 "7D%2C%7B%22name%22%3A%22show_alarms%22%2C%22type%22%3A%22client_action%22%7D%5D");
        UNIT_ASSERT_VALUES_EQUAL(TClientActionUrl::OpenDialogById("super-duper-dialog-id", payload),
                                 "dialog-action://?directives="
                                 "%5B%7B%22name%22%3A%22open_dialog%22%2C%22payload%22%3A%7B%22dialog_id%22%3A%22super-duper-dialog-id%22%2C"
                                 "%22directives%22%3A%5B%7B%22name%22%3A%22new_dialog_session%22%2C%22payload%22%3A%7B%22dialog_id%22%3A%22super-duper-dialog-id%22%7D%2C"
                                 "%22type%22%3A%22server_action%22%7D%5D%2C%22open_fullscreen%22%3A%22true%22%7D%2C%22type%22%3A%22client_action%22%7D%5D");

        UNIT_ASSERT_VALUES_EQUAL(TClientActionUrl::OpenDialogByText("а забабахай мне диалог", payload),
                                 "dialog-action://?directives="
                                 "%5B%7B%22name%22%3A%22type%22%2C%22payload%22%3A%7B%22open_fullscreen%22%3A%22true%22%2C%22text"
                                 "%22%3A%22%D0%B0%20%D0%B7%D0%B0%D0%B1%D0%B0%D0%B1%D0%B0%D1%85%D0%B0%D0%B9%20%D0%BC%D0%BD%D0%B5%20%D0%B4%D0%B8%D0%B0%D0%BB%D0%BE%D0%B3%22%7D%2C%22type%22%3A%22client_action%22%7D%5D");
        NSc::TValue payload1;
        payload1["text"].SetString("аларму давай");
        payload1["somekey"].SetString("somevalue");
        UNIT_ASSERT_VALUES_EQUAL(TClientActionUrl(TClientActionUrl::EType::ShowAlarms, payload1).ToString(),
                                 "dialog-action://"
                                 "?directives=%5B%7B%22name%22%3A%22show_alarms%22%2C%22payload%22%3A%7B%22somekey%22%3A%22somevalue%22"
                                 "%2C%22text%22%3A%22%D0%B0%D0%BB%D0%B0%D1%80%D0%BC%D1%83%20%D0%B4%D0%B0%D0%B2%D0%B0%D0%B9%22%7D%2C%22type%22%3A%22client_action%22%7D%5D");
    }

    Y_UNIT_TEST(TestAddUtmReferrerUnknown) {
        TClientInfo pp(UNKNOWN_APPID);

        UNIT_ASSERT_VALUES_EQUAL(AddUtmReferrer(pp, TStringBuf()), TString());
        UNIT_ASSERT_VALUES_EQUAL(AddUtmReferrer(pp, "https://pornhub.com/"), R"""(https://pornhub.com/)""");
        UNIT_ASSERT_VALUES_EQUAL(AddUtmReferrer(pp, "http://lenta.ru/?read=12345&write=321"), R"""(http://lenta.ru/?read=12345&write=321)""");
    }

    Y_UNIT_TEST(TestAddUtmReferrerPP) {
        TClientInfo pp(SEARCHAPP_APPID);

        UNIT_ASSERT_VALUES_EQUAL(AddUtmReferrer(pp, TStringBuf()), TString());
        UNIT_ASSERT_VALUES_EQUAL(AddUtmReferrer(pp, "https://pornhub.com/"), R"""(https://pornhub.com/?utm_referrer=https%253A%252F%252Fyandex.ru%252Fsearchapp%253Ffrom%253Dalice%2526text%253D)""");
        UNIT_ASSERT_VALUES_EQUAL(AddUtmReferrer(pp, "http://lenta.ru/?read=12345&write=321"), R"""(http://lenta.ru/?read=12345&utm_referrer=https%253A%252F%252Fyandex.ru%252Fsearchapp%253Ffrom%253Dalice%2526text%253D&write=321)""");
    }

    Y_UNIT_TEST(TestAddUtmReferrerYaBro) {
        TClientInfo yabro(YABRO_APPID);

        UNIT_ASSERT_VALUES_EQUAL(AddUtmReferrer(yabro, TStringBuf()), TString());
        UNIT_ASSERT_VALUES_EQUAL(AddUtmReferrer(yabro, "https://pornhub.com/"), R"""(https://pornhub.com/?utm_referrer=https%253A%252F%252Fyandex.ru%252F%253Ffrom%253Dalice)""");
        UNIT_ASSERT_VALUES_EQUAL(AddUtmReferrer(yabro, "http://lenta.ru/?read=12345&write=321"), R"""(http://lenta.ru/?read=12345&utm_referrer=https%253A%252F%252Fyandex.ru%252F%253Ffrom%253Dalice&write=321)""");
    }
}
} // namespace
