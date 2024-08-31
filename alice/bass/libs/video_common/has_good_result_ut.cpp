#include <library/cpp/testing/unittest/registar.h>

#include <alice/bass/libs/video_common/formulas.h>
#include <alice/bass/libs/video_common/has_good_result/factors.h>
#include <alice/bass/libs/video_common/has_good_result/video.sc.h>

#include <library/cpp/scheme/domscheme_traits.h>
#include <library/cpp/scheme/scheme.h>

using namespace NVideoCommon;
using namespace NVideoCommon::NHasGoodResult;

namespace {
auto POS_SAMPLE = NSc::TValue::FromJson(R"({
    "query": "гугл маша и медведь",
    "refined_query": "маша и медведь",
    "results": [
        {
            "genre": "Для детей",
            "name": "Маша и Медведь",
            "rating": 7.17,
            "release_year": 2009,
            "relevance": 104662296,
            "relevance_prediction": 0.0718365593,
            "result": "rel_plus",
            "type": "tv_show",
            "url": "http://www.ivi.ru/watch/masha_i_medved"
        },
        {
            "genre": "Мелодрама",
            "name": "Маша",
            "rating": 6.6,
            "release_year": 2012,
            "relevance": 0.0,
            "relevance_prediction": 0.0,
            "result": "irrel",
            "type": "movie",
            "url": "http://www.ivi.ru/watch/133408/description"
        },
        {
            "genre": "Для детей",
            "name": "Машенька и медведь",
            "rating": 7.12,
            "release_year": 1960,
            "relevance": 0.0,
            "relevance_prediction": 0.0,
            "result": "irrel",
            "type": "movie",
            "url": "http://www.ivi.ru/watch/3244/description"
        },
        {
            "genre": "Для детей",
            "name": "Лиса и медведь",
            "rating": 6.36,
            "release_year": 1975,
            "relevance": 0.0,
            "relevance_prediction": 0.0,
            "result": "irrel",
            "type": "movie",
            "url": "http://www.ivi.ru/watch/33217/description"
        },
        {
            "genre": "Для детей",
            "name": "Трубка и медведь",
            "rating": 6.88,
            "release_year": 1955,
            "relevance": 0.0,
            "relevance_prediction": 0.0,
            "result": "irrel",
            "type": "movie",
            "url": "http://www.ivi.ru/watch/67345/description"
        }
    ]
}
)");

auto NEG_SAMPLE = NSc::TValue::FromJson(R"({
    "query": "легенды бикини боттом мультсериал",
    "refined_query": "легенды бикини боттом",
    "results": [
        {
            "genre": "Для всей семьи",
            "name": "Губка Боб в 3D",
            "rating": 5.52,
            "release_year": 2015,
            "relevance": 103666720,
            "relevance_prediction": 0.05539569939,
            "result": "irrel",
            "type": "movie",
            "url": "http://www.ivi.ru/watch/122610/description"
        },
        {
            "genre": "Для детей",
            "name": "Губка Боб – квадратные штаны",
            "rating": 6.74,
            "release_year": 2004,
            "relevance": 103301432,
            "relevance_prediction": 0.02988637684,
            "result": "irrel",
            "type": "movie",
            "url": "http://www.ivi.ru/watch/124620/description"
        },
        {
            "genre": "Полнометражный",
            "name": "Город героев",
            "rating": 7.93,
            "release_year": 2014,
            "relevance": 103358464,
            "relevance_prediction": 0.0271831004,
            "result": "irrel",
            "type": "movie",
            "url": "http://www.ivi.ru/watch/109905/description"
        },
        {
            "genre": "Драма",
            "name": "Легенда о Коловрате",
            "rating": 6.33,
            "release_year": 2017,
            "relevance": 103125056,
            "relevance_prediction": 0.01622885844,
            "result": "irrel",
            "type": "movie",
            "url": "http://www.ivi.ru/watch/133308/description"
        },
        {
            "genre": "Драма",
            "name": "Викинг + дополнительные материалы",
            "rating": 4.61,
            "release_year": 2016,
            "relevance": 103214904,
            "relevance_prediction": 0.01240952232,
            "result": "irrel",
            "type": "movie",
            "url": "http://www.ivi.ru/watch/133581/description"
        }
    ]
}
)");

TMaybe<double> GetProb(const TFormulas& formulas, NSc::TValue& value) {
    TItem<TSchemeTraits> item(&value);

    TFactors factors;
    factors.FromItem(item);

    return formulas.GetProb(factors);
}

Y_UNIT_TEST_SUITE(THasGoodResultUnitTest) {
    Y_UNIT_TEST(Smoke) {
        auto& formulas = *TFormulas::Instance();
        formulas.Init();

        {
            const auto result = GetProb(formulas, POS_SAMPLE);
            UNIT_ASSERT(result);
            UNIT_ASSERT(*result >= 0.5);
        }

        {
            const auto result = GetProb(formulas, NEG_SAMPLE);
            UNIT_ASSERT(result);
            UNIT_ASSERT(*result < 0.5);
        }
    }
}
} // namespace
