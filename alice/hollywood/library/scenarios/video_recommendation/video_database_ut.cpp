#include "library/cpp/testing/unittest/env.h"
#include "video_database.h"

#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/stream/str.h>

namespace NAlice::NHollywood {

namespace {

const TString RESOURCES_DIR = "alice/hollywood/library/scenarios/video_recommendation/ut/data/";

constexpr TStringBuf DATABASE_PATH = "video_base.txt";
constexpr TStringBuf EMBEDDER_CONFIG_PATH = "bigrams_v_20200101_config.json";
constexpr TStringBuf EMBEDDER_DATA_PATH = "bigrams_v_20200101.dssm";
constexpr TStringBuf SAMPLE_DATABASE_PATH = "sample_video_base.txt";

constexpr size_t RECOMMENDED_COUNT = 3;

TString GetResourcePath(const TStringBuf resourceName) {
    return BinaryPath(RESOURCES_DIR + resourceName);
}

TExpectedFeatures GetScienceFictionFeature() {
    TExpectedFeatures features;
    features.Genre = "science_fiction";

    return features;
}

TExpectedFeatures GetDramaFeature() {
    TExpectedFeatures features;
    features.Genre = "drama";

    return features;
}

TExpectedFeatures GetYearFeature() {
    TExpectedFeatures features;
    features.ReleaseYear.ExactYear = 2000;

    return features;
}

TExpectedFeatures GetCountryFeature() {
    TExpectedFeatures features;
    features.Country = "Франция";

    return features;
}

TExpectedFeatures GetCountryAndGenreFeature() {
    TExpectedFeatures features;
    features.Country = "Франция";
    features.Genre = "drama";

    return features;
}

TExpectedFeatures GetCountryAndYearFeature() {
    TExpectedFeatures features;
    features.Country = "Франция";
    features.ReleaseYear.ExactYear = 2000;

    return features;
}

struct TRecommendationTestSample {
    TExpectedFeatures RequestedFeatures;
    TVector<TString> ExpectedResultNames;
    size_t ExpectedRecommendableCount = 0;
};

const TVector<TRecommendationTestSample> TEST_DATA = {
    {
        GetScienceFictionFeature(),
        {
            "Звёздные войны: Эпизод 6 – Возвращение Джедая",
            "Звёздные войны: Эпизод 5 – Империя наносит ответный удар",
            "Звёздные войны: Эпизод 4 — Новая надежда"
        },
        3
    },
    {
        GetDramaFeature(),
        {
            "Город Бога",
            "Броненосец «Потемкин»"
        },
        2
    },
    {
        GetYearFeature(),
        {
            "Мерцающие огни"
        },
        1
    },
    {
        GetCountryFeature(),
        {
            "Город Бога"
        },
        1
    },
    {
        GetCountryAndGenreFeature(),
        {
            "Город Бога"
        },
        1
    },
    {
        GetCountryAndYearFeature(),
        {
        },
        0
    }
};

} // namespace

Y_UNIT_TEST_SUITE(TVideoDatabaseSuite) {
    Y_UNIT_TEST(TestDummyRecommendation) {
        TVideoDatabase database;
        database.LoadFromPaths(GetResourcePath(SAMPLE_DATABASE_PATH), GetResourcePath(EMBEDDER_DATA_PATH),
                               GetResourcePath(EMBEDDER_CONFIG_PATH));

        for (const TRecommendationTestSample& testSample : TEST_DATA) {
            const auto recommendedItems =
                database.Recommend(testSample.RequestedFeatures, RECOMMENDED_COUNT, /* skipCount= */ 0);

            UNIT_ASSERT_VALUES_EQUAL(recommendedItems.size(), testSample.ExpectedResultNames.size());
            for (size_t i = 0; i < recommendedItems.size(); ++i) {
                UNIT_ASSERT_VALUES_EQUAL(recommendedItems[i].GetName(), testSample.ExpectedResultNames[i]);
            }

            UNIT_ASSERT_VALUES_EQUAL(database.RecommendableItemCount(testSample.RequestedFeatures),
                                     testSample.ExpectedRecommendableCount);
        }
    }

    Y_UNIT_TEST(TestRealDatabase) {
        TVideoDatabase database;
        database.LoadFromPaths(GetResourcePath(DATABASE_PATH), GetResourcePath(EMBEDDER_DATA_PATH),
                               GetResourcePath(EMBEDDER_CONFIG_PATH));

        for (const TRecommendationTestSample& testSample : TEST_DATA) {
            const auto recommendedItems =
                database.Recommend(testSample.RequestedFeatures, RECOMMENDED_COUNT, /* skipCount= */ 0);

            UNIT_ASSERT(!recommendedItems.empty());
        }
    }

    Y_UNIT_TEST(TestEmbedder) {
        TVideoDatabase database;
        database.LoadFromPaths(GetResourcePath(SAMPLE_DATABASE_PATH), GetResourcePath(EMBEDDER_DATA_PATH),
                               GetResourcePath(EMBEDDER_CONFIG_PATH));

        TExpectedFeatures features;
        features.About = "про монстров";

        const auto items = database.Recommend(features, /* gallerySize= */ 2, /* skipCount= */ 0);
        const TVector<TString> expectedItems = {"Корпорация монстров", "Красавица и чудовище"};

        UNIT_ASSERT_VALUES_EQUAL(items.size(), expectedItems.size());
        for (size_t i = 0; i < expectedItems.size(); ++i) {
            UNIT_ASSERT_VALUES_EQUAL(expectedItems[i], items[i].GetName());
        }
    }
}

} // namespace NAlice::NHollywood
