#include <library/cpp/testing/unittest/registar.h>

#include <alice/bass/libs/video_common/formulas.h>
#include <alice/bass/libs/video_common/show_or_gallery/factors.h>
#include <alice/bass/libs/video_common/show_or_gallery/video.sc.h>

#include <library/cpp/scheme/domscheme_traits.h>
#include <library/cpp/scheme/scheme.h>

using namespace NVideoCommon;
using namespace NVideoCommon::NShowOrGallery;

namespace {
auto POS_SAMPLE = NSc::TValue::FromJson(R"(
{
    "query": {
        "freq": 5,
        "text": "3 богатыря и морской царь",
        "search_text": "3 богатыря и морской царь"
    },
    "results": [{
        "genre": "Комедия",
        "name": "Три богатыря и Морской царь",
        "rating": 5.32,
        "release_year": 2016,
        "relevance": 104477192,
        "relevance_prediction": 0.04327267447,
        "type": "movie",
        "url": "http://www.ivi.ru/watch/133312/description"
    }, {
        "genre": "Комедийный",
        "name": "Классная школа",
        "rating": 4.36,
        "release_year": 2013,
        "relevance": 103931176,
        "relevance_prediction": 0.02506328944,
        "type": "tv_show",
        "url": "http://www.ivi.ru/watch/klassnaya-shkola"
    }, {
        "genre": "Приключение",
        "name": "Три богатыря и Шамаханская царица",
        "rating": 6.95,
        "release_year": 2010,
        "relevance": 104098816,
        "relevance_prediction": 0.01834626431,
        "type": "movie",
        "url": "http://www.ivi.ru/watch/45788/description"
    }, {
        "genre": "Драма",
        "name": "Цари и пророки",
        "rating": 0,
        "release_year": 2016,
        "relevance": 102966760,
        "relevance_prediction": 0.01665934769,
        "type": "tv_show",
        "url": "http://www.amediateka.ru/serial/tsari-i-proroki"
    }, {
        "genre": "Русский",
        "name": "Пословицы и поговорки",
        "rating": 0,
        "release_year": 2015,
        "relevance": 103832280,
        "relevance_prediction": 0.01386374783,
        "type": "tv_show",
        "url": "http://www.ivi.ru/watch/poslovitsyi-i-pogovorki"
    }]
}
)");

auto NEG_SAMPLE = NSc::TValue::FromJson(R"(
{
    "query": {
        "freq": 59,
        "text": "аватар",
        "search_text": "аватар"
    },
    "results": [{
        "genre": "Драма",
        "name": "Аватар",
        "rating": 7.95,
        "release_year": 2009,
        "relevance": 104623376,
        "relevance_prediction": 0.1281667322,
        "type": "movie",
        "url": "http://www.ivi.ru/watch/99906/description"
    }, {
        "genre": "Для детей",
        "name": "Аватар: Легенда об Аанге",
        "rating": 8.44,
        "release_year": 2005,
        "relevance": 104275632,
        "relevance_prediction": 0.1082146614,
        "type": "tv_show",
        "url": "http://www.ivi.ru/watch/avatar"
    }, {
        "genre": "Для детей",
        "name": "Аватар: Легенда о Корре",
        "rating": 7.89,
        "release_year": 2012,
        "relevance": 104300768,
        "relevance_prediction": 0.09504834509,
        "type": "tv_show",
        "url": "http://www.ivi.ru/watch/avatar-legenda-o-korre"
    }, {
        "genre": "Мелодрама",
        "name": "Авария",
        "rating": 0,
        "release_year": 2018,
        "relevance": 0.0,
        "relevance_prediction": 0.0,
        "type": "tv_show",
        "url": "http://www.ivi.ru/watch/avariya-2018"
    }, {
        "genre": "Детектив",
        "name": "Авария 1974",
        "rating": 7.47,
        "release_year": 1974,
        "relevance": 0.0,
        "relevance_prediction": 0.0,
        "type": "tv_show",
        "url": "http://www.ivi.ru/watch/avaria"
    }]
}
)");

auto GAME_OF_THRONES = NSc::TValue::FromJson(R"(
{
    "query": {
        "freq": 346,
        "search_text": "игра престолов",
        "text": "игра престолов"
    },
    "results": [{
        "genre": "Драма",
        "name": "Игра престолов",
        "rating": 0,
        "release_year": 2011,
        "relevance": 104828256,
        "relevance_prediction": 0.1355233026,
        "type": "tv_show",
        "url": "http://www.amediateka.ru/serial/igra-prestolov"
    }, {
        "genre": "Документальный",
        "name": "Игра Престолов. В ожидании нового сезона",
        "rating": 0,
        "release_year": 2014,
        "relevance": 0.0,
        "relevance_prediction": 0.0,
        "type": "movie",
        "url": "http://www.amediateka.ru/film/igra-prestolov-v-ozhidanii-novogo-sezona"
    }, {
        "genre": "Документальный",
        "name": "Игра Престолов. Один День из Жизни",
        "rating": 0,
        "release_year": 2015,
        "relevance": 0.0,
        "relevance_prediction": 0.0,
        "type": "movie",
        "url": "http://www.amediateka.ru/film/igra-prestolov-odin-den-iz-zhizni"
    }, {
        "genre": "Документальный",
        "name": "Секреты Игры Престолов",
        "rating": 0,
        "release_year": 2016,
        "relevance": 0.0,
        "relevance_prediction": 0.0,
        "type": "tv_show",
        "url": "http://www.amediateka.ru/serial/sekrety-igry-prestolov"
    }, {
        "genre": "Комедия",
        "name": "Игра",
        "rating": 4.38,
        "release_year": 2008,
        "relevance": 0.0,
        "relevance_prediction": 0.0,
        "type": "movie",
        "url": "http://www.ivi.ru/watch/31366/description"
    }]
}
)");

TMaybe<double> GetProb(const TFormulas& formulas, NSc::TValue& value) {
    TItem<TSchemeTraits> item(&value);

    TFactors factors;
    factors.FromItem(item);

    return formulas.GetProb(factors);
}

Y_UNIT_TEST_SUITE(TShowOrGalleryUnitTest) {
    Y_UNIT_TEST(Smoke) {
        auto& formulas = *TFormulas::Instance();
        formulas.Init();

        {
            const auto result = GetProb(formulas, POS_SAMPLE);
            UNIT_ASSERT(result);
            UNIT_ASSERT_C(*result >= 0.5, *result);
        }

        {
            const auto result = GetProb(formulas, NEG_SAMPLE);
            UNIT_ASSERT(result);
            UNIT_ASSERT_C(*result < 0.5, *result);
        }

        {
            // This case is way too important, and this check should never fail :(
            const auto result = GetProb(formulas, GAME_OF_THRONES);
            UNIT_ASSERT(result);
            UNIT_ASSERT_C(*result >= 0.5, *result);
        }
    }
}
} // namespace
