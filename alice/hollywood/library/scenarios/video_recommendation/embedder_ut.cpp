#include "video_database.h"

#include <library/cpp/dot_product/dot_product.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/stream/str.h>

namespace NAlice::NHollywood {

namespace {

const TString RESOURCES_DIR = "alice/hollywood/library/scenarios/video_recommendation/ut/data/";

constexpr TStringBuf EMBEDDER_CONFIG_PATH = "bigrams_v_20200101_config.json";
constexpr TStringBuf EMBEDDER_DATA_PATH = "bigrams_v_20200101.dssm";

struct TMovieInfo {
    TString Title;
    TString Host;
    TString Path;
};

struct TTestCase {
    TString Query;
    TString ExpectedAnswer;
};

const TVector<TMovieInfo> MOVIES = {
    {"Побег из Шоушенка", "https://www.kinopoisk.ru", "/film/326/"},
    {"Форрест Гамп", "https://www.kinopoisk.ru", "/film/448/"},
    {"Римские каникулы", "https://www.kinopoisk.ru", "/film/497/"},
    {"Хористы", "https://www.kinopoisk.ru", "/film/51481/"},
    {"Зверополис", "https://www.kinopoisk.ru", "/film/775276/"},
};

const TVector<TTestCase> TESTS = {
    {"фильм про тюрьму", "Побег из Шоушенка"},
    {"фильм где беги форрест беги", "Форрест Гамп"},
    {"фильм про любовь", "Римские каникулы"},
    {"фильм про школу", "Хористы"},
    {"фильм про зверушек", "Зверополис"},
};

TString GetResourcePath(const TStringBuf resourceName) {
    return BinaryPath(RESOURCES_DIR + resourceName);
}

size_t FindBestMatch(const TVector<float>& queryEmbedding, const TVector<TVector<float>>& movieEmbeddings) {
    size_t bestMovieIndex = 0;
    float bestMovieScore = -100.f;
    for (size_t movieIndex = 0; movieIndex < movieEmbeddings.size(); ++movieIndex) {
        const auto& movieEmbedding = movieEmbeddings[movieIndex];

        const float score = DotProduct(queryEmbedding.data(), movieEmbedding.data(), movieEmbedding.size());

        if (score > bestMovieScore) {
            bestMovieScore = score;
            bestMovieIndex = movieIndex;
        }
    }

    return bestMovieIndex;
}

} // namespace

Y_UNIT_TEST_SUITE(TEmbedderSuite) {
    Y_UNIT_TEST(TestEmbedder) {
        TMovieInfoEmbedder embedder;
        embedder.Load(GetResourcePath(EMBEDDER_DATA_PATH), GetResourcePath(EMBEDDER_CONFIG_PATH));

        TVector<TVector<float>> movieEmbeddings;
        for (const auto& movie : MOVIES) {
            movieEmbeddings.push_back(embedder.EmbedMovie(movie.Title, movie.Host, movie.Path));
        }

        for (const auto& testCase : TESTS) {
            const TVector<float> queryEmbedding = embedder.EmbedQuery(testCase.Query);

            const size_t movieIndex = FindBestMatch(queryEmbedding, movieEmbeddings);
            UNIT_ASSERT_VALUES_EQUAL(testCase.ExpectedAnswer, MOVIES[movieIndex].Title);
        }
    }
}

} // namespace NAlice::NHollywood
