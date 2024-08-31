#include "direct_gallery.h"

#include <alice/bass/ut/helpers.h>
#include <alice/library/unittest/ut_helpers.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/algorithm.h>

using namespace NBASS;
using namespace NBASS::NDirectGallery;
using namespace NTestingHelpers;

namespace {

const auto ADS_RESPONSE = NSc::TValue::FromJson(R"(
{
  "_url": [
    {
      "url": "yabs.yandex.ru/code/620060"
    }
  ],
  "direct_halfpremium": [
    {
      "bid": "72057603561862846",
      "body": "Гарантия подлинности. Более 3 600 моделей \u0007[колец\u0007]. Доставка за 1 День!",
      "domain": "karat-jewell.com",
      "fav_domain": "karat-jewell.com",
      "link_tail": "link_tail_1",
      "title": "\u0007[Купить\u0007] \u0007[золотое\u0007] \u0007[кольцо\u0007] – Белгородский Ювелирный Завод",
      "url": "ads_url_1",
      "warning": "",
      "warning_size": "",
    }
  ],
  "direct_premium": [
    {
      "bid": "72057603561862846",
      "body": "Гарантия подлинности. Более 3 600 моделей \u0007[колец\u0007]. Доставка за 1 День!",
      "domain": "karat-jewell.com",
      "fav_domain": "karat-jewell.com",
      "link_tail": "link_tail_1",
      "title": "\u0007[Купить\u0007] \u0007[золотое\u0007] \u0007[кольцо\u0007] – Белгородский Ювелирный Завод",
      "url": "ads_url_1",
      "warning": "",
      "warning_size": "",
    },
    {
      "bid": "6806991849",
      "body": "Большой выбор \u0007[золотых\u0007] \u0007[колец\u0007] в ювелирном магазине SUNLIGHT! Бесплатная доставка!",
      "domain": "sunlight.net",
      "link_tail": "ads_link_tail_2",
      "title": "\u0007[Купите\u0007] \u0007[золотое\u0007] \u0007[кольцо\u0007] в SUNLIGHT! – Все за бесценок до -80%",
      "url": "ads_url_2",
      "warning": "",
      "warning_size": "",
    }
  ],
  "stat": [
    {
      "link_head": "http://yabs.yandex.ru/count/link_head_",
      "hit_counter": "https://yabs.yandex.ru/hitcount/?text=hitcount_text"
    }
  ]
})");

const auto BASS_DIRECT_RESPONSE = NSc::TValue::FromJson(R"(
{
    "blocks" : [
        {
            "card_template" : "direct_gallery",
            "data" : {
                "card_height" : 210,
                "cards_with_button" : 1,
                "direct_items" : [
                    {
                        "disclaimer" : "",
                        "domain_name" : "karat-jewell.com",
                        "green_url" : "karat-jewell.com",
                        "text" : "Гарантия подлинности. Более 3 600 моделей колец. Доставка за 1 День!",
                        "title" : "Купить золотое кольцо – Белгородский Ювелирный Завод",
                        "url" : "ads_url_1"
                    },
                    {
                        "disclaimer" : "",
                        "domain_name" : null,
                        "green_url" : "sunlight.net",
                        "text" : "Большой выбор золотых колец в ювелирном магазине SUNLIGHT! Бесплатная доставка!",
                        "title" : "Купите золотое кольцо в SUNLIGHT! – Все за бесценок до -80%",
                        "url" : "ads_url_2"
                    }
                ],
                "has_disclaimer" : 0,
                "has_large_disclaimer" : 0,
                "serp_url" : "viewport://?direct_page=620060&l10n=ru-RU&noreask=1&query_source=alice&text=%D0%BA%D1%83%D0%BF%D0%B8%D1%82%D1%8C&viewport_id=serp"
            },
            "type" : "div_card"
        },
        {
            "data" : null,
            "phrase_id" : "direct_gallery_invitation_message",
            "type" : "text_card"
        },
        {
            "type" : "stop_listening"
        },
        {
            "data" : {
                "IsFinished" : false,
                "ObjectTypeName" : "TDirectGalleryHitConfirmContinuation",
                "State" : "{\"context\":{\"form\":{\"name\":\"unit_test_form_handler.default\",\"slots\":[]}},\"hit_counter\":\"https://yabs.yandex.ru/hitcount/?text=hitcount_text\",\"link_head\":\"http://yabs.yandex.ru/count/link_head_\",\"link_tails\":[\"link_tail_1\",\"ads_link_tail_2\"]}"
            },
            "type" : "commit_candidate"
        }
    ],
    "form" : {
        "name" : "personal_assistant.scenarios.direct_gallery",
        "slots" : [
            {
                "name" : "direct_items",
                "optional" : true,
                "type" : "direct_items",
                "value" : [
                    {
                        "disclaimer" : "",
                        "domain_name" : "karat-jewell.com",
                        "green_url" : "karat-jewell.com",
                        "text" : "Гарантия подлинности. Более 3 600 моделей колец. Доставка за 1 День!",
                        "title" : "Купить золотое кольцо – Белгородский Ювелирный Завод",
                        "url" : "ads_url_1"
                    },
                    {
                        "disclaimer" : "",
                        "domain_name" : null,
                        "green_url" : "sunlight.net",
                        "text" : "Большой выбор золотых колец в ювелирном магазине SUNLIGHT! Бесплатная доставка!",
                        "title" : "Купите золотое кольцо в SUNLIGHT! – Все за бесценок до -80%",
                        "url" : "ads_url_2"
                    }
                ]
            }
        ]
    }
})");

Y_UNIT_TEST_SUITE(BassDirectGalleryUnitTest) {
    Y_UNIT_TEST(ParseHtml) {
        {
            const TString rawText("\u0007[Пластиковые\u0007] \u0007[окна\u0007] &laquo;REHAU&raquo; &ndash; от 3.800 ₽ за кв. метр");
            const TString clearText("\u0007[Пластиковые\u0007] \u0007[окна\u0007] «REHAU» – от 3.800 ₽ за кв. метр");

            UNIT_ASSERT_EQUAL(ReplaceHtmlEntitiesInText(rawText), clearText);
        }
        {
            const TString rawText("<a href=\"https://https://oknadomkom.ru\">&lt;Пластиковые окна&gt;</a>");
            const TString clearText("<Пластиковые окна>");

            UNIT_ASSERT_EQUAL(ReplaceHtmlEntitiesInText(rawText), clearText);
        }
    }

    Y_UNIT_TEST(ConfirmHit) {
        NTestingHelpers::TRequestJson request;
        const auto ctx = MakeContext(NSc::TValue(request));
        auto builder = TDirectGalleryBuilder::Create(*ctx);
        UNIT_ASSERT(builder);
        NSc::TValue searchResult;
        searchResult["banner"]["data"] = ADS_RESPONSE;
        auto newCtx = builder->Build("купить", searchResult);
        UNIT_ASSERT(newCtx);
        UNIT_ASSERT(EqualJson(BASS_DIRECT_RESPONSE, newCtx->ToJson()));
        const auto blocks = newCtx->GetBlocks();
        const auto commitBlock =
            FindIf(blocks, [](const auto& block) { return block.get()["type"] == "commit_candidate"; });
        UNIT_ASSERT(commitBlock != blocks.end());
    }
}

} // namespace
